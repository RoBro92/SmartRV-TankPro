#include "controller_link.h"
#include "ui_custom.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <algorithm>
#include <cmath>
#include <limits.h>
#include <vector>

namespace {

constexpr uint16_t kControllerPort = 60505;
constexpr uint32_t kControllerStaleMs = 10000;
constexpr uint32_t kControllerStateTimeoutMs = 10000;
constexpr uint32_t kPairingAdvertTimeoutMs = 8000;
// Allow full-size UDP packets (up to typical MTU) so JSON payloads aren't truncated.
constexpr size_t kMaxPacketSize = 2048;
constexpr const char *kFreshIdKey = "fresh_ctrl_id";
constexpr const char *kWasteIdKey = "waste_ctrl_id";
constexpr uint32_t kConfigPendingTimeoutMs = 5000;
constexpr const char *kFreshCfgInitKey = "fr_init";
constexpr const char *kWasteCfgInitKey = "wa_init";
constexpr const char *kFreshFillStopKey = "fr_fill";
constexpr const char *kFreshDrainStopKey = "fr_drain";
constexpr const char *kFreshFreezeEnKey = "fr_frzen";
constexpr const char *kFreshFreezeThKey = "fr_frthr";
constexpr const char *kFreshEmptyVKey = "fr_empty";
constexpr const char *kFreshFullVKey = "fr_full";
constexpr const char *kWasteFillStopKey = "wa_fill";
constexpr const char *kWasteDrainStopKey = "wa_drain";
constexpr const char *kWasteFreezeEnKey = "wa_frzen";
constexpr const char *kWasteFreezeThKey = "wa_frthr";
constexpr const char *kWasteEmptyVKey = "wa_empty";
constexpr const char *kWasteFullVKey = "wa_full";
constexpr const char *kFreshDrainTimeoutKey = "fr_drain_to";
constexpr const char *kWasteDrainTimeoutKey = "wa_drain_to";
constexpr const char *kFreshSafetyKey = "fr_safe";
constexpr const char *kWasteSafetyKey = "wa_safe";
constexpr const char *kFreshValveKey = "fr_valve";
constexpr const char *kWasteValveKey = "wa_valve";

Preferences *prefs_handle = nullptr;
WiFiUDP udp;
bool udp_started = false;
bool state_dirty = false;
bool fresh_pair_ack = false;
bool waste_pair_ack = false;
struct UnpairPending {
    bool active = false;
    tank_role_t role = TANK_ROLE_NONE;
    String controller_id;
    uint32_t timeout_ms = 0;
};
UnpairPending unpair_pending;

struct ControllerRecord {
    String id;
    tank_role_t reported_role = TANK_ROLE_NONE;
    IPAddress ip;
    String mac;
    String version;
    uint32_t last_seen_ms = 0;
    int16_t wifi_signal_dbm = INT16_MIN;
    uint32_t uptime_s = 0;
    float level = NAN;
    float temp_c = NAN;
    bool leak = false;
    bool has_leak = false;
    bool freeze_enabled = false;
    bool has_freeze_enabled = false;
    float freeze_threshold_c = NAN;
    bool has_freeze_threshold = false;
    bool fill_in_progress = false;
    bool drain_in_progress = false;
    bool fault_active = false;
    uint16_t fault_code = CYD_FAULT_INVALID;
    bool has_fault_code = false;
    String fault_description;
    float fill_stop_level = NAN;
    bool has_fill_stop = false;
    float drain_stop_level = NAN;
    bool has_drain_stop = false;
    float empty_v = NAN;
    bool has_empty_v = false;
    float full_v = NAN;
    bool has_full_v = false;
    bool safety_override = false;
    bool has_safety_override = false;
    bool valve_override = false;
    bool has_valve_override = false;
    String status;
    bool paired_flag = false;
    uint32_t pairing_token = 0;
    bool has_pairing_token = false;
    bool pairing_active = false;
    uint32_t last_advert_ts = 0;
    uint32_t last_state_ts = 0;
    bool online = false;
};

std::vector<ControllerRecord> controllers;
String stored_fresh_id;
String stored_waste_id;

tank_role_t role_from_string(const String &role);
const char *role_to_string(tank_role_t role);
static void controller_link_send_config_dump_request(const String &id, tank_role_t role, const IPAddress &ip);

struct PendingField {
    bool pending = false;
    uint32_t last_ms = 0;
    float desired = 0.0f;
};

struct PendingConfig {
    PendingField fill_stop;
    PendingField drain_stop;
    PendingField freeze_enabled;
    PendingField freeze_threshold;
    PendingField drain_timeout;
    PendingField empty_volts;
    PendingField full_volts;
    PendingField safety_override;
    PendingField valve_override;
};

PendingConfig pending_configs[3];  // index by tank_role_t (NONE/FRESH/WASTE)

struct TankConfig {
    bool initialized = false;
    float fill_stop = 90.0f;
    float drain_stop = 10.0f;
    bool freeze_enabled = false;
    float freeze_threshold_c = 2.0f;
    float empty_volts = 0.5f;
    float full_volts = 2.5f;
    uint32_t drain_timeout_ms = 45000;
    bool safety_override = false;
    bool valve_override = false;
};

// Defaults mirrored to controllers; user changes from the CYD UI persist in NVS.
TankConfig fresh_config;
TankConfig waste_config;
static uint32_t cmd_window_start_ms[3] = {0, 0, 0};
static uint8_t cmd_window_count[3] = {0, 0, 0};
constexpr uint32_t kCmdWindowMs = 2000;     // allow at most two commands per 2 seconds per role
constexpr uint8_t kCmdMaxPerWindow = 2;

tank_role_t role_from_string(const String &role) {
    if (role.equalsIgnoreCase("fresh")) return TANK_ROLE_FRESH;
    if (role.equalsIgnoreCase("waste")) return TANK_ROLE_WASTE;
    return TANK_ROLE_NONE;
}

PendingConfig &pending_for_role(tank_role_t role) {
    switch (role) {
        case TANK_ROLE_FRESH: return pending_configs[1];
        case TANK_ROLE_WASTE: return pending_configs[2];
        default: return pending_configs[0];
    }
}

TankConfig &config_for_role(tank_role_t role) {
    switch (role) {
        case TANK_ROLE_FRESH: return fresh_config;
        case TANK_ROLE_WASTE: return waste_config;
        default: return fresh_config;  // fallback
    }
}

// Returns the mutable tank state for a role or nullptr if unassigned.
tank_state_t *tank_state_for_role(tank_role_t role) {
    switch (role) {
        case TANK_ROLE_FRESH: return &cyd_state.fresh;
        case TANK_ROLE_WASTE: return &cyd_state.waste;
        default: return nullptr;
    }
}

void save_config_to_nvs(tank_role_t role) {
    if (!prefs_handle) return;
    TankConfig &cfg = config_for_role(role);
    const bool fresh = role == TANK_ROLE_FRESH;
    prefs_handle->putBool(fresh ? kFreshCfgInitKey : kWasteCfgInitKey, cfg.initialized);
    prefs_handle->putFloat(fresh ? kFreshFillStopKey : kWasteFillStopKey, cfg.fill_stop);
    prefs_handle->putFloat(fresh ? kFreshDrainStopKey : kWasteDrainStopKey, cfg.drain_stop);
    prefs_handle->putBool(fresh ? kFreshFreezeEnKey : kWasteFreezeEnKey, cfg.freeze_enabled);
    prefs_handle->putFloat(fresh ? kFreshFreezeThKey : kWasteFreezeThKey, cfg.freeze_threshold_c);
    prefs_handle->putFloat(fresh ? kFreshEmptyVKey : kWasteEmptyVKey, cfg.empty_volts);
    prefs_handle->putFloat(fresh ? kFreshFullVKey : kWasteFullVKey, cfg.full_volts);
    prefs_handle->putUInt(fresh ? kFreshDrainTimeoutKey : kWasteDrainTimeoutKey, cfg.drain_timeout_ms);
    prefs_handle->putBool(fresh ? kFreshSafetyKey : kWasteSafetyKey, cfg.safety_override);
    prefs_handle->putBool(fresh ? kFreshValveKey : kWasteValveKey, cfg.valve_override);
    Serial.printf("[cfg] saved %s config: fill=%.1f drain=%.1f freeze=%s thr=%.2f empty=%.3f full=%.3f dto=%ums\n",
                  role_to_string(role), cfg.fill_stop, cfg.drain_stop, cfg.freeze_enabled ? "true" : "false",
                  cfg.freeze_threshold_c, cfg.empty_volts, cfg.full_volts, (unsigned)cfg.drain_timeout_ms);
}

void load_configs_from_nvs() {
    if (!prefs_handle) return;
    fresh_config.initialized = prefs_handle->getBool(kFreshCfgInitKey, false);
    fresh_config.fill_stop = prefs_handle->getFloat(kFreshFillStopKey, 90.0f);
    fresh_config.drain_stop = prefs_handle->getFloat(kFreshDrainStopKey, 10.0f);
    fresh_config.freeze_enabled = prefs_handle->getBool(kFreshFreezeEnKey, false);
    fresh_config.freeze_threshold_c = prefs_handle->getFloat(kFreshFreezeThKey, 2.0f);
    fresh_config.empty_volts = prefs_handle->getFloat(kFreshEmptyVKey, 0.5f);
    fresh_config.full_volts = prefs_handle->getFloat(kFreshFullVKey, 2.5f);
    fresh_config.drain_timeout_ms = prefs_handle->getUInt(kFreshDrainTimeoutKey, 45000);
    fresh_config.safety_override = prefs_handle->getBool(kFreshSafetyKey, false);
    fresh_config.valve_override = prefs_handle->getBool(kFreshValveKey, false);
    Serial.printf("[cfg] loaded Fresh config: init=%s fill=%.1f drain=%.1f freeze=%s thr=%.2f empty=%.3f full=%.3f dto=%ums\n",
                  fresh_config.initialized ? "true" : "false", fresh_config.fill_stop, fresh_config.drain_stop,
                  fresh_config.freeze_enabled ? "true" : "false", fresh_config.freeze_threshold_c,
                  fresh_config.empty_volts, fresh_config.full_volts, (unsigned)fresh_config.drain_timeout_ms);

    waste_config.initialized = prefs_handle->getBool(kWasteCfgInitKey, false);
    waste_config.fill_stop = prefs_handle->getFloat(kWasteFillStopKey, 90.0f);
    waste_config.drain_stop = prefs_handle->getFloat(kWasteDrainStopKey, 10.0f);
    waste_config.freeze_enabled = prefs_handle->getBool(kWasteFreezeEnKey, false);
    waste_config.freeze_threshold_c = prefs_handle->getFloat(kWasteFreezeThKey, 2.0f);
    waste_config.empty_volts = prefs_handle->getFloat(kWasteEmptyVKey, 0.5f);
    waste_config.full_volts = prefs_handle->getFloat(kWasteFullVKey, 2.5f);
    waste_config.drain_timeout_ms = prefs_handle->getUInt(kWasteDrainTimeoutKey, 45000);
    waste_config.safety_override = prefs_handle->getBool(kWasteSafetyKey, false);
    waste_config.valve_override = prefs_handle->getBool(kWasteValveKey, false);
    Serial.printf("[cfg] loaded Waste config: init=%s fill=%.1f drain=%.1f freeze=%s thr=%.2f empty=%.3f full=%.3f dto=%ums\n",
                  waste_config.initialized ? "true" : "false", waste_config.fill_stop, waste_config.drain_stop,
                  waste_config.freeze_enabled ? "true" : "false", waste_config.freeze_threshold_c,
                  waste_config.empty_volts, waste_config.full_volts, (unsigned)waste_config.drain_timeout_ms);
}

const char *role_to_string(tank_role_t role) {
    switch (role) {
        case TANK_ROLE_FRESH: return "Fresh";
        case TANK_ROLE_WASTE: return "Waste";
        default: return "Unassigned";
    }
}

ControllerRecord *find_controller(const String &id) {
    auto it = std::find_if(controllers.begin(), controllers.end(),
                           [&](const ControllerRecord &rec) { return rec.id == id; });
    if (it == controllers.end()) return nullptr;
    return &(*it);
}

ControllerRecord *get_or_create_controller(const String &id) {
    if (id.isEmpty()) return nullptr;
    ControllerRecord *rec = find_controller(id);
    if (rec) return rec;
    ControllerRecord blank;
    blank.id = id;
    controllers.push_back(blank);
    return &controllers.back();
}

void load_assignments_from_nvs() {
    if (!prefs_handle) return;
    stored_fresh_id = prefs_handle->getString(kFreshIdKey, "");
    stored_waste_id = prefs_handle->getString(kWasteIdKey, "");
    if (stored_fresh_id.length() > 0) {
        snprintf(cyd_state.fresh.diag_id, sizeof(cyd_state.fresh.diag_id), "%s", stored_fresh_id.c_str());
        snprintf(cyd_state.fresh.diag_status, sizeof(cyd_state.fresh.diag_status), "Not connected");
        snprintf(cyd_state.fresh.diag_role, sizeof(cyd_state.fresh.diag_role), "%s", role_to_string(TANK_ROLE_FRESH));
    }
    if (stored_waste_id.length() > 0) {
        snprintf(cyd_state.waste.diag_id, sizeof(cyd_state.waste.diag_id), "%s", stored_waste_id.c_str());
        snprintf(cyd_state.waste.diag_status, sizeof(cyd_state.waste.diag_status), "Not connected");
        snprintf(cyd_state.waste.diag_role, sizeof(cyd_state.waste.diag_role), "%s", role_to_string(TANK_ROLE_WASTE));
    }
}

void persist_assignment(const String &fresh_id, const String &waste_id) {
    if (!prefs_handle) return;
    prefs_handle->putString(kFreshIdKey, fresh_id);
    prefs_handle->putString(kWasteIdKey, waste_id);
}

void assign_if_needed(const String &id, tank_role_t role) {
    if (id.isEmpty()) return;
    if (role == TANK_ROLE_FRESH && stored_fresh_id.isEmpty()) {
        stored_fresh_id = id;
        persist_assignment(id, "");
        snprintf(cyd_state.fresh.diag_id, sizeof(cyd_state.fresh.diag_id), "%s", id.c_str());
        snprintf(cyd_state.fresh.diag_role, sizeof(cyd_state.fresh.diag_role), "%s", role_to_string(TANK_ROLE_FRESH));
    } else if (role == TANK_ROLE_WASTE && stored_waste_id.isEmpty()) {
        stored_waste_id = id;
        persist_assignment("", id);
        snprintf(cyd_state.waste.diag_id, sizeof(cyd_state.waste.diag_id), "%s", id.c_str());
        snprintf(cyd_state.waste.diag_role, sizeof(cyd_state.waste.diag_role), "%s", role_to_string(TANK_ROLE_WASTE));
    }
}

void start_udp() {
    if (udp_started || WiFi.status() != WL_CONNECTED) return;
    if (udp.begin(kControllerPort)) {
        udp_started = true;
        Serial.printf("[ctrl] listening on UDP %u\n", kControllerPort);
    }
}

void stop_udp() {
    if (!udp_started) return;
    udp.stop();
    udp_started = false;
}

void map_status(tank_state_t &tank, const ControllerRecord &rec) {
    if (rec.status.equalsIgnoreCase("fill")) {
        tank.status = TANK_STATUS_FILL;
    } else if (rec.status.equalsIgnoreCase("drain")) {
        tank.status = TANK_STATUS_DRAIN;
    } else if (rec.status.equalsIgnoreCase("fault") || rec.fault_active) {
        tank.status = TANK_STATUS_FAULT;
    } else if (rec.fill_in_progress) {
        tank.status = TANK_STATUS_FILL;
    } else if (rec.drain_in_progress) {
        tank.status = TANK_STATUS_DRAIN;
    } else if (rec.status.equalsIgnoreCase("pairing")) {
        tank.status = TANK_STATUS_OK;
    } else if (!rec.status.isEmpty()) {
        tank.status = TANK_STATUS_OK;
    }
}

uint8_t clamp_percent(float value) {
    if (isnan(value) || value < 0) return CYD_LEVEL_INVALID;
    if (value > 100) value = 100;
    return static_cast<uint8_t>(lroundf(value));
}

bool nearly_equal(float a, float b, float tol = 0.1f) {
    return fabsf(a - b) <= tol;
}

bool should_apply_config(PendingField &pf, float remote_value, const char *label, tank_role_t role) {
    const uint32_t now = millis();
    if (!pf.pending) return true;
    if (nearly_equal(remote_value, pf.desired)) {
        pf.pending = false;
        Serial.printf("[cfg] %s %s synchronised (remote=%.2f)\n", role_to_string(role), label, remote_value);
        return true;
    }
    if (now - pf.last_ms <= kConfigPendingTimeoutMs) {
        Serial.printf("[cfg] ignoring stale remote %s %s=%.2f (pending user=%.2f)\n", role_to_string(role), label,
                      remote_value, pf.desired);
        return false;
    }
    Serial.printf("[cfg] pending timeout for %s %s; reverting to controller value=%.2f\n", role_to_string(role), label,
                  remote_value);
    pf.pending = false;
    pf.desired = remote_value;
    return true;
}

uint8_t freeze_setting_from_threshold(float threshold_c, bool enabled) {
    if (!enabled) return 0;
    if (isnan(threshold_c)) return CYD_SETTING_INVALID;
    int setting = static_cast<int>(lroundf(threshold_c));
    if (setting < 0) setting = 0;
    if (setting > 5) setting = 5;
    return static_cast<uint8_t>(setting);
}

uint16_t mv_from_volts(float v) {
    if (isnan(v) || v <= 0) return CYD_VOLT_INVALID;
    int mv = static_cast<int>(lroundf(v * 1000.0f));
    if (mv < 0) mv = 0;
    if (mv > 5000) mv = 5000;
    return static_cast<uint16_t>(mv);
}

void apply_config_to_state(tank_state_t &tank, const TankConfig &cfg, tank_role_t role) {
    if (role == TANK_ROLE_FRESH) {
        tank.stop_level_percent = clamp_percent(cfg.fill_stop);
    } else if (role == TANK_ROLE_WASTE) {
        tank.stop_level_percent = clamp_percent(cfg.drain_stop);
    }
    tank.freeze_enabled = cfg.freeze_enabled;
    tank.freeze_setting = freeze_setting_from_threshold(cfg.freeze_threshold_c, cfg.freeze_enabled);
    tank.full_voltage_mv = mv_from_volts(cfg.full_volts);
    tank.empty_voltage_mv = mv_from_volts(cfg.empty_volts);
    tank.safety_override_enabled = cfg.safety_override;
    tank.valve_override_enabled = cfg.valve_override;
}

void apply_controller_to_tank(tank_state_t &tank, const ControllerRecord &rec, tank_role_t target_role) {
    // Clear local lost-connection fault (code 9) when fresh state resumes.
    if (tank.fault_code == 9) {
        tank.fault_code = 0;
        snprintf(tank.fault_description, sizeof(tank.fault_description), "No Active Fault");
    }
    tank.paired = true;
    tank.role = static_cast<int8_t>(target_role);
    if (!isnan(rec.level)) tank.level_percent = clamp_percent(rec.level);
    if (!isnan(rec.temp_c) && rec.temp_c > -200.0f && rec.temp_c < 200.0f) tank.temp_c = rec.temp_c;
    map_status(tank, rec);
    if (rec.has_leak) tank.leak = rec.leak;
    // Config values are driven by CYD; do not override from controller state.
    if (rec.has_fault_code) {
        tank.fault_code = rec.fault_code;
        snprintf(tank.fault_description, sizeof(tank.fault_description), "%s", rec.fault_description.c_str());
    }
    // Config stop levels owned by CYD; state packets no longer supply them.
    if (rec.has_full_v) tank.full_voltage_mv = mv_from_volts(rec.full_v);
    if (rec.has_empty_v) tank.empty_voltage_mv = mv_from_volts(rec.empty_v);
    if (rec.has_safety_override) tank.safety_override_enabled = rec.safety_override;
    if (rec.has_valve_override) tank.valve_override_enabled = rec.valve_override;
    if (rec.mac.length() > 0) snprintf(tank.diag_mac, sizeof(tank.diag_mac), "%s", rec.mac.c_str());
    snprintf(tank.diag_status, sizeof(tank.diag_status), "Connected");
    snprintf(tank.diag_role, sizeof(tank.diag_role), "%s", role_to_string(target_role));
    if (rec.ip != IPAddress()) snprintf(tank.diag_ip, sizeof(tank.diag_ip), "%s", rec.ip.toString().c_str());
    if (rec.id.length() > 0) snprintf(tank.diag_id, sizeof(tank.diag_id), "%s", rec.id.c_str());
    if (rec.version.length() > 0) snprintf(tank.diag_version, sizeof(tank.diag_version), "%s", rec.version.c_str());
    if (rec.wifi_signal_dbm != INT16_MIN) tank.diag_signal_dbm = rec.wifi_signal_dbm;
    if (rec.uptime_s > 0) tank.diag_uptime_s = rec.uptime_s;
}

void mark_tank_disconnected(tank_state_t &tank, const String &assigned_id, tank_role_t role) {
    tank.paired = false;
    tank.level_percent = CYD_LEVEL_INVALID;
    tank.temp_c = NAN;
    tank.status = TANK_STATUS_OK;
    tank.leak = false;
    tank.freeze_enabled = false;
    tank.freeze_setting = CYD_SETTING_INVALID;
    tank.fault_code = CYD_FAULT_INVALID;
    tank.fault_description[0] = '\0';
    tank.stop_level_percent = CYD_SETTING_INVALID;
    tank.safety_override_enabled = false;
    tank.valve_override_enabled = false;
    tank.full_voltage_mv = CYD_VOLT_INVALID;
    tank.empty_voltage_mv = CYD_VOLT_INVALID;
    tank.diag_signal_dbm = 0;
    tank.diag_uptime_s = 0;
    tank.diag_ip[0] = '\0';
    tank.diag_mac[0] = '\0';
    tank.diag_version[0] = '\0';
    snprintf(tank.diag_status, sizeof(tank.diag_status), "Not connected");
    snprintf(tank.diag_role, sizeof(tank.diag_role), "%s", role_to_string(role));
    if (assigned_id.length() > 0) {
        snprintf(tank.diag_id, sizeof(tank.diag_id), "%s", assigned_id.c_str());
    } else {
        tank.diag_id[0] = '\0';
    }
}

void mark_tank_offline(tank_state_t &tank, const String &assigned_id, tank_role_t role) {
    tank.paired = true;  // still assigned, just offline
    tank.level_percent = CYD_LEVEL_INVALID;
    tank.temp_c = NAN;
    tank.status = TANK_STATUS_FAULT;
    tank.leak = false;
    tank.freeze_enabled = false;
    tank.freeze_setting = CYD_SETTING_INVALID;
    tank.fault_code = 9;  // CYD-local: controller lost
    if (assigned_id.length() > 0) {
        snprintf(tank.fault_description, sizeof(tank.fault_description),
                 "Lost connection to tank controller \"%s\". Please check the controller.", assigned_id.c_str());
    } else {
        snprintf(tank.fault_description, sizeof(tank.fault_description),
                 "Lost connection to tank controller. Please check the controller.");
    }
    tank.stop_level_percent = CYD_SETTING_INVALID;
    tank.safety_override_enabled = false;
    tank.valve_override_enabled = false;
    tank.full_voltage_mv = CYD_VOLT_INVALID;
    tank.empty_voltage_mv = CYD_VOLT_INVALID;
    tank.diag_signal_dbm = 0;
    tank.diag_uptime_s = 0;
    tank.diag_ip[0] = '\0';
    tank.diag_mac[0] = '\0';
    tank.diag_version[0] = '\0';
    snprintf(tank.diag_status, sizeof(tank.diag_status), "Offline");
    snprintf(tank.diag_role, sizeof(tank.diag_role), "%s", role_to_string(role));
    if (assigned_id.length() > 0) {
        snprintf(tank.diag_id, sizeof(tank.diag_id), "%s", assigned_id.c_str());
    }
}

void apply_to_ui_if_needed() {
    if (!state_dirty) return;
    state_dirty = false;
    cyd_state_apply_to_home_screen();
    cyd_state_apply_to_fresh_screen();
    cyd_state_apply_to_waste_screen();
    cyd_state_apply_to_freshfaults_screen();
    cyd_state_apply_to_wastefaults_screen();
    cyd_state_apply_to_freshsettings_screen();
    cyd_state_apply_to_wastesettings_screen();
    cyd_state_apply_to_cydsettings_diag_overlay();
    cyd_state_apply_to_freshsettings_diag_overlay();
    cyd_state_apply_to_wastesettings_diag_overlay();
}

void process_assignment_and_state(ControllerRecord &rec) {
    assign_if_needed(rec.id, rec.reported_role);

    const bool fresh_match = !stored_fresh_id.isEmpty() && rec.id == stored_fresh_id;
    const bool waste_match = !stored_waste_id.isEmpty() && rec.id == stored_waste_id;

    if (fresh_match) {
        apply_controller_to_tank(cyd_state.fresh, rec, TANK_ROLE_FRESH);
        state_dirty = true;
    }
    if (waste_match) {
        apply_controller_to_tank(cyd_state.waste, rec, TANK_ROLE_WASTE);
        state_dirty = true;
    }

    // Auto-assign if not stored yet.
    if (!fresh_match && stored_fresh_id.isEmpty() && rec.reported_role == TANK_ROLE_FRESH) {
        stored_fresh_id = rec.id;
        persist_assignment(rec.id, "");
        apply_controller_to_tank(cyd_state.fresh, rec, TANK_ROLE_FRESH);
        state_dirty = true;
    } else if (!waste_match && stored_waste_id.isEmpty() && rec.reported_role == TANK_ROLE_WASTE) {
        stored_waste_id = rec.id;
        persist_assignment("", rec.id);
        apply_controller_to_tank(cyd_state.waste, rec, TANK_ROLE_WASTE);
        state_dirty = true;
    }
}

void check_for_stale_controllers() {
    const uint32_t now = millis();
    if (!stored_fresh_id.isEmpty()) {
        ControllerRecord *fresh = find_controller(stored_fresh_id);
        if (fresh && fresh->online && now - fresh->last_state_ts > kControllerStateTimeoutMs) {
            fresh->online = false;
            Serial.printf("[ctrl] id=%s state timeout; marking offline\n", fresh->id.c_str());
        }
    }
    if (!stored_waste_id.isEmpty()) {
        ControllerRecord *waste = find_controller(stored_waste_id);
        if (waste && waste->online && now - waste->last_state_ts > kControllerStateTimeoutMs) {
            waste->online = false;
            Serial.printf("[ctrl] id=%s state timeout; marking offline\n", waste->id.c_str());
        }
    }
}

void controller_link_handle_pair_ack(const String &id, tank_role_t role, bool paired, const IPAddress &remote_ip) {
    ControllerRecord *rec = get_or_create_controller(id);
    if (rec) {
        rec->last_seen_ms = millis();
        rec->ip = remote_ip;
        rec->reported_role = role;
        rec->paired_flag = paired;
        rec->pairing_active = false;
    }
    bool matched = false;
    if (role == TANK_ROLE_FRESH && !stored_fresh_id.isEmpty() && id == stored_fresh_id) {
        fresh_pair_ack = paired;
        cyd_state.fresh.paired = paired;
        snprintf(cyd_state.fresh.diag_status, sizeof(cyd_state.fresh.diag_status), paired ? "Paired" : "Not connected");
        state_dirty = true;
        matched = true;
    } else if (role == TANK_ROLE_WASTE && !stored_waste_id.isEmpty() && id == stored_waste_id) {
        waste_pair_ack = paired;
        cyd_state.waste.paired = paired;
        snprintf(cyd_state.waste.diag_status, sizeof(cyd_state.waste.diag_status), paired ? "Paired" : "Not connected");
        state_dirty = true;
        matched = true;
    }
    if (matched) {
        pairing_ui_on_pair_ack(id, role);
    }
    Serial.printf("[pair] ack id=%s role=%s paired=%s\n", id.c_str(), role_to_string(role), paired ? "true" : "false");
    TankConfig &cfg = config_for_role(role);
    if (!cfg.initialized) {
        controller_link_send_config_dump_request(id, role, remote_ip);
    } else {
        ControllerConfigUpdate full;
        full.has_fill_stop = true;
        full.fill_stop_percent = clamp_percent(role == TANK_ROLE_FRESH ? cfg.fill_stop : cfg.fill_stop);
        full.has_drain_stop = true;
        full.drain_stop_percent = clamp_percent(role == TANK_ROLE_WASTE ? cfg.drain_stop : cfg.drain_stop);
        full.has_freeze_enabled = true;
        full.freeze_enabled = cfg.freeze_enabled;
        full.has_freeze_threshold = true;
        full.freeze_threshold_c = cfg.freeze_threshold_c;
        full.has_drain_timeout = true;
        full.drain_timeout_ms = cfg.drain_timeout_ms;
        full.has_empty_volts = true;
        full.empty_volts = cfg.empty_volts;
        full.has_full_volts = true;
        full.full_volts = cfg.full_volts;
        controller_link_send_config_update(role, full);
    }
}

void controller_link_handle_unpair_ack(const String &id) {
    bool cleared = false;
    if (!stored_fresh_id.isEmpty() && id == stored_fresh_id) {
        stored_fresh_id = "";
        persist_assignment(stored_fresh_id, stored_waste_id);
        mark_tank_disconnected(cyd_state.fresh, "", TANK_ROLE_FRESH);
        fresh_pair_ack = false;
        cleared = true;
        pairing_ui_on_unpair_complete(TANK_ROLE_FRESH);
    }
    if (!stored_waste_id.isEmpty() && id == stored_waste_id) {
        stored_waste_id = "";
        persist_assignment(stored_fresh_id, stored_waste_id);
        mark_tank_disconnected(cyd_state.waste, "", TANK_ROLE_WASTE);
        waste_pair_ack = false;
        cleared = true;
        pairing_ui_on_unpair_complete(TANK_ROLE_WASTE);
    }
    if (ControllerRecord *rec = find_controller(id)) {
        rec->pairing_active = false;
    }
    if (cleared) {
        state_dirty = true;
        unpair_pending.active = false;
    }
    Serial.printf("[unpair] ack for %s (cleared=%s)\n", id.c_str(), cleared ? "yes" : "no");
}

void handle_packet(const char *buf, size_t len, const IPAddress &remote_ip) {
    if (len < 2) return;  // ignore empty/very short datagrams
    if (len >= kMaxPacketSize) {
        Serial.printf("[ctrl] dropping truncated packet (len=%u, buf=%u)\n", static_cast<unsigned>(len),
                      static_cast<unsigned>(kMaxPacketSize));
        return;
    }
    // Debug: show the first part of the payload so we can see pairing adverts.
    size_t preview_len = len > 200 ? 200 : len;
    Serial.printf("[ctrl] rx %u/%u bytes: %.*s\n", static_cast<unsigned>(len), static_cast<unsigned>(kMaxPacketSize),
                  static_cast<int>(preview_len), buf);
    // Expected payload (JSON):
    // {
    //   "type": "tank_controller",
    //   "event": "state",
    //   "id": "<TankPro-XXXX>",
    //   "mac": "...",
    //   "role": "Fresh"|"Waste"|"Unassigned",
    //   "status": "ok"|"fill"|"drain"|"fault"|"pairing",
    //   "level": float percent or -1 when invalid,
    //   "temp_c": float Celsius or < -200 when invalid,
    //   "leak": bool,
    //   "freeze_enabled": bool,
    //   "freeze_threshold_c": float,
    //   "fill_in_progress": bool,
    //   "drain_in_progress": bool,
    //   "fault_active": bool,
    //   "fault_code": int,
    //   "fault_description": "...",
    //   "fill_stop_level": float,
    //   "drain_stop_level": float,
    //   "empty_v": float volts,
    //   "full_v": float volts,
    //   "safety_override": bool,
    //   "valve_override": bool,
    //   "paired": bool,
    //   "wifi_signal_dbm": int,
    //   "uptime_s": uint32,
    //   "version": "...",
    //   "ip": "x.x.x.x"
    // }
    // Large enough to parse full state payloads without truncation.
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, buf, len);
    if (err) {
        static uint32_t last_log_ms = 0;
        const uint32_t now = millis();
        if (now - last_log_ms > 2000) {
            Serial.printf("[ctrl] json parse failed: %s (len=%u)\n", err.c_str(), static_cast<unsigned>(len));
            last_log_ms = now;
        }
        return;
    }
    const char *type = doc["type"] | "";
    const char *event = doc["event"] | "";
    if (strcmp(type, "tank_controller") == 0 && strcmp(event, "pair_ack") == 0) {
        const String id = doc["id"] | "";
        const String role_str = doc["role"] | "";
        const bool paired = doc["paired"] | false;
        tank_role_t role = role_from_string(role_str);
        controller_link_handle_pair_ack(id, role, paired, remote_ip);
        return;
    }
    if (strcmp(type, "tank_controller") == 0 && strcmp(event, "config_dump") == 0) {
        const String id = doc["id"] | "";
        const String role_str = doc["role"] | "";
        tank_role_t role = role_from_string(role_str);
        TankConfig &conf = config_for_role(role);
        if (role == TANK_ROLE_NONE || (role == TANK_ROLE_FRESH && id != stored_fresh_id) ||
            (role == TANK_ROLE_WASTE && id != stored_waste_id)) {
            Serial.printf("[cfg] ignoring config_dump from %s role=%s (not assigned)\n", id.c_str(), role_str.c_str());
            return;
        }
        if (conf.initialized) {
            Serial.printf("[cfg] config_dump ignored for %s (already initialized)\n", role_to_string(role));
            return;
        }
        ControllerConfigUpdate cfg;
        if (doc.containsKey("params")) {
            JsonObject params = doc["params"].as<JsonObject>();
            if (params.containsKey("fill_stop_percent")) {
                cfg.has_fill_stop = true;
                cfg.fill_stop_percent = params["fill_stop_percent"];
            }
            if (params.containsKey("drain_stop_percent")) {
                cfg.has_drain_stop = true;
                cfg.drain_stop_percent = params["drain_stop_percent"];
            }
            if (params.containsKey("drain_timeout_ms")) {
                cfg.has_drain_timeout = true;
                cfg.drain_timeout_ms = params["drain_timeout_ms"];
            }
            if (params.containsKey("freeze_enabled")) {
                cfg.has_freeze_enabled = true;
                cfg.freeze_enabled = params["freeze_enabled"];
            }
            if (params.containsKey("freeze_threshold_c")) {
                cfg.has_freeze_threshold = true;
                cfg.freeze_threshold_c = params["freeze_threshold_c"];
            }
            if (params.containsKey("level_empty_volts")) {
                cfg.has_empty_volts = true;
                cfg.empty_volts = params["level_empty_volts"];
            }
            if (params.containsKey("level_full_volts")) {
                cfg.has_full_volts = true;
                cfg.full_volts = params["level_full_volts"];
            }
            if (params.containsKey("safety_override")) {
                cfg.has_safety_override = true;
                cfg.safety_override = params["safety_override"];
            }
            if (params.containsKey("valve_override")) {
                cfg.has_valve_override = true;
                cfg.valve_override = params["valve_override"];
            }
        }
        controller_link_apply_local_config(role, cfg, false);
        config_for_role(role).initialized = true;
        save_config_to_nvs(role);
        Serial.printf("[cfg] imported config_dump for %s from %s\n", role_to_string(role), id.c_str());
        return;
    }
    if (strcmp(type, "tank_controller") == 0 && strcmp(event, "config_ack") == 0) {
        const String id = doc["id"] | "";
        const String role_str = doc["role"] | "";
        const bool applied = doc["applied"] | false;
        const String error = doc["error"] | "";
        tank_role_t role = role_from_string(role_str);
        ControllerRecord *rec = get_or_create_controller(id);
        if (rec) {
            rec->last_seen_ms = millis();
            rec->ip = remote_ip;
            rec->reported_role = role;
        }
        PendingConfig &pending = pending_for_role(role);
        if (!applied && error.length() > 0) {
            Serial.printf("[cfg] config_ack error from %s: %s\n", id.c_str(), error.c_str());
            pending.fill_stop.pending = false;
            pending.drain_stop.pending = false;
            pending.freeze_enabled.pending = false;
            pending.freeze_threshold.pending = false;
            pending.drain_timeout.pending = false;
            pending.empty_volts.pending = false;
            pending.full_volts.pending = false;
        } else {
            Serial.printf("[cfg] config_ack from %s role=%s applied=%s\n", id.c_str(), role_to_string(role),
                          applied ? "true" : "false");
        }
    if (doc.containsKey("params")) {
        JsonObject params = doc["params"].as<JsonObject>();
        ControllerConfigUpdate cfg;
        if (params.containsKey("fill_stop_percent")) {
            cfg.has_fill_stop = true;
            cfg.fill_stop_percent = params["fill_stop_percent"];
            pending.fill_stop.pending = false;
        }
        if (params.containsKey("drain_stop_percent")) {
            cfg.has_drain_stop = true;
            cfg.drain_stop_percent = params["drain_stop_percent"];
            pending.drain_stop.pending = false;
        }
        if (params.containsKey("drain_timeout_ms")) {
            cfg.has_drain_timeout = true;
            cfg.drain_timeout_ms = params["drain_timeout_ms"];
            pending.drain_timeout.pending = false;
        }
        if (params.containsKey("freeze_enabled")) {
            cfg.has_freeze_enabled = true;
            cfg.freeze_enabled = params["freeze_enabled"];
            pending.freeze_enabled.pending = false;
        }
        if (params.containsKey("freeze_threshold_c")) {
            cfg.has_freeze_threshold = true;
            cfg.freeze_threshold_c = params["freeze_threshold_c"];
            pending.freeze_threshold.pending = false;
        }
        if (params.containsKey("level_empty_volts")) {
            cfg.has_empty_volts = true;
            cfg.empty_volts = params["level_empty_volts"];
            pending.empty_volts.pending = false;
        }
        if (params.containsKey("level_full_volts")) {
            cfg.has_full_volts = true;
            cfg.full_volts = params["level_full_volts"];
            pending.full_volts.pending = false;
        }
        if (params.containsKey("safety_override")) {
            cfg.has_safety_override = true;
            cfg.safety_override = params["safety_override"];
            pending.safety_override.pending = false;
        }
        if (params.containsKey("valve_override")) {
            cfg.has_valve_override = true;
            cfg.valve_override = params["valve_override"];
            pending.valve_override.pending = false;
        }
        Serial.printf("[cfg] config_ack id=%s role=%s applied=%s at t=%lums\n", id.c_str(), role_to_string(role),
                      applied ? "true" : "false", static_cast<unsigned long>(millis()));
        controller_link_apply_local_config(role, cfg, false);
        process_assignment_and_state(*rec);
    }
    return;
}
    if (strcmp(type, "tank_controller") == 0 && strcmp(event, "unpair_ack") == 0) {
        const String id = doc["id"] | "";
        controller_link_handle_unpair_ack(id);
        return;
    }
    if (strcmp(type, "tank_controller") == 0 && strcmp(event, "pairing_advert") == 0) {
        // Minimal registration so the UI can see controllers even before full state arrives.
        const String identifier = doc["id"] | doc["identifier"] | "";
        ControllerRecord *rec = get_or_create_controller(identifier);
        if (rec) {
            rec->last_seen_ms = millis();
            rec->ip = remote_ip;
            const String mac = doc["mac"] | "";
            if (mac.length() > 0) rec->mac = mac;
            rec->reported_role = role_from_string(String(doc["role"] | ""));
            rec->paired_flag = false;
            if (doc.containsKey("token")) {
                rec->pairing_token = doc["token"];
                rec->has_pairing_token = true;
            } else {
                rec->has_pairing_token = false;
            }
            rec->pairing_active = true;
            rec->last_advert_ts = millis();
            process_assignment_and_state(*rec);
            Serial.printf("[pair] advert id=%s mac=%s role=%s token=%u\n", identifier.c_str(), mac.c_str(),
                          role_to_string(rec->reported_role),
                          rec->has_pairing_token ? rec->pairing_token : 0);
        }
        return;
    }
    if (strcmp(type, "tank_controller") != 0 || strcmp(event, "state") != 0) return;

    const String id = doc["id"] | "";
    ControllerRecord *rec = get_or_create_controller(id);
    if (!rec) return;
    rec->last_seen_ms = millis();
    rec->ip = remote_ip;
    rec->mac = String(doc["mac"] | "");
    rec->version = String(doc["version"] | "");
    rec->wifi_signal_dbm = doc["wifi_signal_dbm"] | INT16_MIN;
    rec->uptime_s = doc["uptime_s"] | 0;
    rec->status = String(doc["status"] | "");
    rec->paired_flag = doc["paired"] | false;
    const bool was_online = rec->online;
    if (!rec->online) {
        Serial.printf("[ctrl] id=%s state received; marking online\n", rec->id.c_str());
    }
    rec->online = true;
    rec->last_state_ts = millis();
    if (!rec->status.equalsIgnoreCase("pairing")) {
        if (rec->pairing_active) {
            Serial.printf("[pair] id=%s pairing_active=false (status=%s)\n", rec->id.c_str(), rec->status.c_str());
        }
        rec->pairing_active = false;
    }

    rec->reported_role = role_from_string(String(doc["role"] | ""));
    if (doc.containsKey("level")) rec->level = doc["level"];
    if (doc.containsKey("temp_c")) rec->temp_c = doc["temp_c"];
    if (doc.containsKey("leak")) {
        rec->leak = doc["leak"];
        rec->has_leak = true;
    }
    if (doc.containsKey("fill_in_progress")) rec->fill_in_progress = doc["fill_in_progress"];
    if (doc.containsKey("drain_in_progress")) rec->drain_in_progress = doc["drain_in_progress"];
    if (doc.containsKey("fault_active")) rec->fault_active = doc["fault_active"];
    if (doc.containsKey("fault_code")) {
        rec->fault_code = doc["fault_code"];
        rec->has_fault_code = true;
        rec->fault_active = rec->fault_code != 0;
    }
    if (doc.containsKey("fault_description")) rec->fault_description = String(doc["fault_description"].as<const char *>());
    if (doc.containsKey("valve_override")) rec->valve_override = doc["valve_override"];

    process_assignment_and_state(*rec);

    // If controller just came online, push full config to ensure it matches CYD.
    if (!was_online) {
        ControllerConfigUpdate full;
        if (rec->reported_role == TANK_ROLE_FRESH) {
            TankConfig &cfg = fresh_config;
            full.has_fill_stop = true;
            full.fill_stop_percent = clamp_percent(cfg.fill_stop);
            full.has_drain_stop = true;
            full.drain_stop_percent = clamp_percent(cfg.drain_stop);
            full.has_freeze_enabled = true;
            full.freeze_enabled = cfg.freeze_enabled;
            full.has_freeze_threshold = true;
            full.freeze_threshold_c = cfg.freeze_threshold_c;
            full.has_drain_timeout = true;
            full.drain_timeout_ms = cfg.drain_timeout_ms;
            full.has_empty_volts = true;
            full.empty_volts = cfg.empty_volts;
            full.has_full_volts = true;
            full.full_volts = cfg.full_volts;
            full.has_safety_override = true;
            full.safety_override = cfg.safety_override;
            full.has_valve_override = true;
            full.valve_override = cfg.valve_override;
            controller_link_send_config_update(TANK_ROLE_FRESH, full);
        } else if (rec->reported_role == TANK_ROLE_WASTE) {
            TankConfig &cfg = waste_config;
            full.has_fill_stop = true;
            full.fill_stop_percent = clamp_percent(cfg.fill_stop);
            full.has_drain_stop = true;
            full.drain_stop_percent = clamp_percent(cfg.drain_stop);
            full.has_freeze_enabled = true;
            full.freeze_enabled = cfg.freeze_enabled;
            full.has_freeze_threshold = true;
            full.freeze_threshold_c = cfg.freeze_threshold_c;
            full.has_drain_timeout = true;
            full.drain_timeout_ms = cfg.drain_timeout_ms;
            full.has_empty_volts = true;
            full.empty_volts = cfg.empty_volts;
            full.has_full_volts = true;
            full.full_volts = cfg.full_volts;
            full.has_safety_override = true;
            full.safety_override = cfg.safety_override;
            full.has_valve_override = true;
            full.valve_override = cfg.valve_override;
            controller_link_send_config_update(TANK_ROLE_WASTE, full);
        }
    }
}

static void controller_link_send_config_dump_request(const String &id, tank_role_t role, const IPAddress &ip) {
    if (id.isEmpty() || role == TANK_ROLE_NONE) return;
    IPAddress target_ip = ip == IPAddress() ? IPAddress(255, 255, 255, 255) : ip;
    StaticJsonDocument<192> doc;
    doc["type"] = "cyd_config";
    doc["event"] = "config_dump_request";
    doc["id"] = id;
    doc["role"] = role_to_string(role);
    char payload[192];
    const size_t written = serializeJson(doc, payload, sizeof(payload));
    if (!written || written >= sizeof(payload)) return;
    WiFiUDP sender;
    if (!sender.begin(0)) return;
    sender.beginPacket(target_ip, kControllerPort);
    sender.write(reinterpret_cast<const uint8_t *>(payload), written);
    sender.endPacket();
    sender.stop();
    Serial.printf("[cfg] sent config_dump_request to %s role=%s\n", id.c_str(), role_to_string(role));
}

}  // namespace

std::vector<ControllerSummary> controller_link_get_controllers() {
    std::vector<ControllerSummary> out;
    const uint32_t now = millis();
    size_t pairing_active_count = 0;
    for (const auto &rec : controllers) {
        // Only surface fairly recent controllers to the UI.
        if (rec.last_seen_ms > 0 && now - rec.last_seen_ms <= kControllerStaleMs * 2) {
            // Skip controllers already assigned to fresh or waste.
            if ((!stored_fresh_id.isEmpty() && rec.id == stored_fresh_id) ||
                (!stored_waste_id.isEmpty() && rec.id == stored_waste_id)) {
                continue;
            }
            if (!rec.pairing_active) continue;
            pairing_active_count++;
            ControllerSummary summary;
            summary.id = rec.id;
            summary.reported_role = rec.reported_role;
            summary.paired_flag = rec.paired_flag;
            summary.wifi_signal_dbm = rec.wifi_signal_dbm;
            summary.last_seen_ms = rec.last_seen_ms;
            out.push_back(summary);
        }
    }
    Serial.printf("[pair] registry has %u controllers, pairing_active %u\n",
                  static_cast<unsigned>(controllers.size()), static_cast<unsigned>(pairing_active_count));
    return out;
}

bool controller_link_assign_controller(const String &id, tank_role_t role) {
    if (id.isEmpty() || role == TANK_ROLE_NONE) return false;
    if (role == TANK_ROLE_FRESH) {
        stored_fresh_id = id;
        persist_assignment(id, stored_waste_id);
    } else if (role == TANK_ROLE_WASTE) {
        stored_waste_id = id;
        persist_assignment(stored_fresh_id, id);
    }

    ControllerRecord *rec = find_controller(id);
    if (rec) {
        rec->reported_role = role;
        process_assignment_and_state(*rec);
    } else {
        if (role == TANK_ROLE_FRESH) {
            mark_tank_disconnected(cyd_state.fresh, stored_fresh_id, TANK_ROLE_FRESH);
        } else if (role == TANK_ROLE_WASTE) {
            mark_tank_disconnected(cyd_state.waste, stored_waste_id, TANK_ROLE_WASTE);
        }
        state_dirty = true;
    }

    return true;
}

void controller_link_send_pair_request(const String &id, tank_role_t role, const String &cyd_id) {
    if (id.isEmpty() || role == TANK_ROLE_NONE) return;
    ControllerRecord *rec = find_controller(id);
    IPAddress target_ip;
    if (rec && rec->ip != IPAddress()) {
        target_ip = rec->ip;
    } else {
        target_ip = IPAddress(255, 255, 255, 255);
    }
    WiFiUDP sender;
    if (!sender.begin(0)) return;
    StaticJsonDocument<256> doc;
    doc["type"] = "cyd_pair";
    doc["event"] = "request";
    doc["id"] = id;
    doc["role"] = role == TANK_ROLE_FRESH ? "Fresh" : "Waste";
    if (!cyd_id.isEmpty()) doc["cyd_id"] = cyd_id;
    char payload[256];
    size_t written = serializeJson(doc, payload, sizeof(payload));
    if (written > 0) {
        sender.beginPacket(target_ip, kControllerPort);
        sender.write(reinterpret_cast<const uint8_t *>(payload), written);
        sender.endPacket();
    }
    sender.stop();
}

void controller_link_send_config_update(tank_role_t role, const ControllerConfigUpdate &cfg) {
    if (role != TANK_ROLE_FRESH && role != TANK_ROLE_WASTE) return;
    if (!cfg.has_fill_stop && !cfg.has_drain_stop && !cfg.has_freeze_enabled && !cfg.has_freeze_threshold &&
        !cfg.has_drain_timeout && !cfg.has_empty_volts && !cfg.has_full_volts && !cfg.has_safety_override &&
        !cfg.has_valve_override) {
        Serial.println("[cfg] no fields set; skipping config_update");
        return;
    }
    const String target_id = (role == TANK_ROLE_FRESH) ? stored_fresh_id : stored_waste_id;
    if (target_id.isEmpty()) {
        Serial.printf("[cfg] no controller assigned for %s\n", role_to_string(role));
        return;
    }
    PendingConfig &pending = pending_for_role(role);
    const uint32_t now = millis();
    if (cfg.has_fill_stop) {
        pending.fill_stop.pending = true;
        pending.fill_stop.last_ms = now;
        pending.fill_stop.desired = cfg.fill_stop_percent;
        Serial.printf("[cfg] user changed %s fill_stop_percent -> %u (pending)\n", role_to_string(role),
                      cfg.fill_stop_percent);
    }
    if (cfg.has_drain_stop) {
        pending.drain_stop.pending = true;
        pending.drain_stop.last_ms = now;
        pending.drain_stop.desired = cfg.drain_stop_percent;
        Serial.printf("[cfg] user changed %s drain_stop_percent -> %u (pending)\n", role_to_string(role),
                      cfg.drain_stop_percent);
    }
    if (cfg.has_freeze_enabled) {
        pending.freeze_enabled.pending = true;
        pending.freeze_enabled.last_ms = now;
        pending.freeze_enabled.desired = cfg.freeze_enabled ? 1.0f : 0.0f;
        Serial.printf("[cfg] user changed %s freeze_enabled -> %s (pending)\n", role_to_string(role),
                      cfg.freeze_enabled ? "true" : "false");
    }
    if (cfg.has_freeze_threshold) {
        pending.freeze_threshold.pending = true;
        pending.freeze_threshold.last_ms = now;
        pending.freeze_threshold.desired = cfg.freeze_threshold_c;
        Serial.printf("[cfg] user changed %s freeze_threshold_c -> %.2f (pending)\n", role_to_string(role),
                      cfg.freeze_threshold_c);
    }
    if (cfg.has_drain_timeout) {
        pending.drain_timeout.pending = true;
        pending.drain_timeout.last_ms = now;
        pending.drain_timeout.desired = static_cast<float>(cfg.drain_timeout_ms);
        Serial.printf("[cfg] user changed %s drain_timeout_ms -> %u (pending)\n", role_to_string(role),
                      cfg.drain_timeout_ms);
    }
    if (cfg.has_empty_volts) {
        pending.empty_volts.pending = true;
        pending.empty_volts.last_ms = now;
        pending.empty_volts.desired = cfg.empty_volts;
        Serial.printf("[cfg] user changed %s empty_volts -> %.3f (pending)\n", role_to_string(role),
                      cfg.empty_volts);
    }
    if (cfg.has_full_volts) {
        pending.full_volts.pending = true;
        pending.full_volts.last_ms = now;
        pending.full_volts.desired = cfg.full_volts;
        Serial.printf("[cfg] user changed %s full_volts -> %.3f (pending)\n", role_to_string(role),
                      cfg.full_volts);
    }
    if (cfg.has_safety_override) {
        pending.safety_override.pending = true;
        pending.safety_override.last_ms = now;
        pending.safety_override.desired = cfg.safety_override ? 1.0f : 0.0f;
        Serial.printf("[cfg] user changed %s safety_override -> %s (pending)\n", role_to_string(role),
                      cfg.safety_override ? "true" : "false");
    }
    if (cfg.has_valve_override) {
        pending.valve_override.pending = true;
        pending.valve_override.last_ms = now;
        pending.valve_override.desired = cfg.valve_override ? 1.0f : 0.0f;
        Serial.printf("[cfg] user changed %s valve_override -> %s (pending)\n", role_to_string(role),
                      cfg.valve_override ? "true" : "false");
    }
    ControllerRecord *rec = find_controller(target_id);
    IPAddress target_ip = IPAddress(255, 255, 255, 255);
    if (rec && rec->ip != IPAddress()) target_ip = rec->ip;

    StaticJsonDocument<512> doc;
    doc["type"] = "cyd_config";
    doc["event"] = "config_update";
    doc["id"] = target_id;
    doc["role"] = role_to_string(role);
    JsonObject params = doc.createNestedObject("params");
    if (cfg.has_fill_stop) params["fill_stop_percent"] = cfg.fill_stop_percent;
    if (cfg.has_drain_stop) params["drain_stop_percent"] = cfg.drain_stop_percent;
    if (cfg.has_freeze_enabled) params["freeze_enabled"] = cfg.freeze_enabled;
    if (cfg.has_freeze_threshold) params["freeze_threshold_c"] = cfg.freeze_threshold_c;
    if (cfg.has_drain_timeout) params["drain_timeout_ms"] = cfg.drain_timeout_ms;
    if (cfg.has_empty_volts) params["level_empty_volts"] = cfg.empty_volts;
    if (cfg.has_full_volts) params["level_full_volts"] = cfg.full_volts;
    if (cfg.has_safety_override) params["safety_override"] = cfg.safety_override;
    if (cfg.has_valve_override) params["valve_override"] = cfg.valve_override;

    char payload[512];
    const size_t written = serializeJson(doc, payload, sizeof(payload));
    if (written == 0 || written >= sizeof(payload)) {
        Serial.println("[cfg] failed to build config_update payload");
        return;
    }

    WiFiUDP sender;
    if (!sender.begin(0)) return;
    sender.beginPacket(target_ip, kControllerPort);
    sender.write(reinterpret_cast<const uint8_t *>(payload), written);
    sender.endPacket();
    sender.stop();

    Serial.printf("[cfg] sending config_update to %s role=%s", target_id.c_str(), role_to_string(role));
    if (cfg.has_fill_stop) Serial.printf(" fill_stop=%u", cfg.fill_stop_percent);
    if (cfg.has_drain_stop) Serial.printf(" drain_stop=%u", cfg.drain_stop_percent);
    if (cfg.has_freeze_enabled) Serial.printf(" freeze_en=%s", cfg.freeze_enabled ? "true" : "false");
    if (cfg.has_freeze_threshold) Serial.printf(" freeze_thresh=%.2f", cfg.freeze_threshold_c);
    if (cfg.has_drain_timeout) Serial.printf(" drain_timeout=%u", cfg.drain_timeout_ms);
    if (cfg.has_empty_volts) Serial.printf(" empty_v=%.3f", cfg.empty_volts);
    if (cfg.has_full_volts) Serial.printf(" full_v=%.3f", cfg.full_volts);
    if (cfg.has_safety_override) Serial.printf(" safety=%s", cfg.safety_override ? "true" : "false");
    if (cfg.has_valve_override) Serial.printf(" valve=%s", cfg.valve_override ? "true" : "false");
    Serial.println();
}

void controller_link_send_command(tank_role_t role, const char *action) {
    if (!action || (role != TANK_ROLE_FRESH && role != TANK_ROLE_WASTE)) return;
    const uint8_t idx = static_cast<uint8_t>(role);
    const uint32_t now = millis();
    if (now - cmd_window_start_ms[idx] > kCmdWindowMs) {
        cmd_window_start_ms[idx] = now;
        cmd_window_count[idx] = 0;
    }
    if (cmd_window_count[idx] >= kCmdMaxPerWindow) {
        Serial.printf("[cmd] throttled %s for role=%s (count=%u in %ums window)\n",
                      action, role_to_string(role), cmd_window_count[idx], kCmdWindowMs);
        return;
    }
    cmd_window_count[idx]++;

    const String target_id = (role == TANK_ROLE_FRESH) ? stored_fresh_id : stored_waste_id;
    if (target_id.isEmpty()) {
        Serial.printf("[cmd] no controller assigned for %s\n", role_to_string(role));
        return;
    }
    ControllerRecord *rec = find_controller(target_id);
    if (rec && strcmp(action, "clear_faults") == 0) {
        rec->fault_code = 0;
        rec->fault_description = "No Active Fault";
        rec->fault_active = false;
        rec->has_fault_code = true;
    }
    IPAddress target_ip = IPAddress(255, 255, 255, 255);
    if (rec && rec->ip != IPAddress()) target_ip = rec->ip;

    // Optimistically clear the local fault view so the UI reflects the button tap immediately.
    if (strcmp(action, "clear_faults") == 0) {
        if (tank_state_t *tank = tank_state_for_role(role)) {
            tank->fault_code = 0;
            snprintf(tank->fault_description, sizeof(tank->fault_description), "No Active Fault");
            if (tank->status == TANK_STATUS_FAULT) tank->status = TANK_STATUS_OK;
            state_dirty = true;
        }
    }

    StaticJsonDocument<256> doc;
    doc["type"] = "cyd_cmd";
    doc["event"] = action;
    doc["id"] = target_id;
    doc["role"] = role_to_string(role);
    char payload[256];
    const size_t written = serializeJson(doc, payload, sizeof(payload));
    if (written == 0 || written >= sizeof(payload)) {
        Serial.println("[cmd] failed to build payload");
        return;
    }
    WiFiUDP sender;
    if (!sender.begin(0)) return;
    sender.beginPacket(target_ip, kControllerPort);
    sender.write(reinterpret_cast<const uint8_t *>(payload), written);
    sender.endPacket();
    sender.stop();
    Serial.printf("[cmd] sent %s to %s role=%s\n", action, target_id.c_str(), role_to_string(role));
}

void controller_link_send_calibrate_empty(tank_role_t role) {
    controller_link_send_command(role, "calibrate_empty");
}

void controller_link_send_calibrate_full(tank_role_t role) {
    controller_link_send_command(role, "calibrate_full");
}

void controller_link_send_restart(tank_role_t role) {
    controller_link_send_command(role, "restart");
}

void controller_link_apply_local_config(tank_role_t role, const ControllerConfigUpdate &cfg, bool send_to_controller) {
    TankConfig &conf = config_for_role(role);
    bool changed = false;
    if (send_to_controller) {
        Serial.printf("[cfg] apply_local role=%s at t=%lums\n", role_to_string(role),
                      static_cast<unsigned long>(millis()));
    }
    if (cfg.has_fill_stop) {
        conf.fill_stop = cfg.fill_stop_percent;
        changed = true;
    }
    if (cfg.has_drain_stop) {
        conf.drain_stop = cfg.drain_stop_percent;
        changed = true;
    }
    if (cfg.has_freeze_enabled) {
        conf.freeze_enabled = cfg.freeze_enabled;
        changed = true;
    }
    if (cfg.has_freeze_threshold) {
        conf.freeze_threshold_c = cfg.freeze_threshold_c;
        changed = true;
    }
    if (cfg.has_drain_timeout) {
        conf.drain_timeout_ms = cfg.drain_timeout_ms;
        changed = true;
    }
    if (cfg.has_empty_volts) {
        conf.empty_volts = cfg.empty_volts;
        changed = true;
    }
    if (cfg.has_full_volts) {
        conf.full_volts = cfg.full_volts;
        changed = true;
    }
    if (cfg.has_safety_override) {
        conf.safety_override = cfg.safety_override;
        changed = true;
    }
    if (cfg.has_valve_override) {
        conf.valve_override = cfg.valve_override;
        changed = true;
    }
    if (!changed) return;
    conf.initialized = true;
    if (role == TANK_ROLE_FRESH) {
        apply_config_to_state(cyd_state.fresh, conf, TANK_ROLE_FRESH);
    } else if (role == TANK_ROLE_WASTE) {
        apply_config_to_state(cyd_state.waste, conf, TANK_ROLE_WASTE);
    }
    save_config_to_nvs(role);
    state_dirty = true;
    if (send_to_controller) {
        Serial.printf("[cfg] sending config_update role=%s at t=%lums\n", role_to_string(role),
                      static_cast<unsigned long>(millis()));
        controller_link_send_config_update(role, cfg);
    }
}

void controller_link_request_unpair(tank_role_t role) {
    String target_id;
    if (role == TANK_ROLE_FRESH) target_id = stored_fresh_id;
    else if (role == TANK_ROLE_WASTE) target_id = stored_waste_id;

    if (target_id.isEmpty()) {
        Serial.printf("[unpair] no controller assigned to %s\n", role_to_string(role));
        // Clear local state anyway.
        if (role == TANK_ROLE_FRESH) {
            mark_tank_disconnected(cyd_state.fresh, "", TANK_ROLE_FRESH);
            stored_fresh_id = "";
        } else if (role == TANK_ROLE_WASTE) {
            mark_tank_disconnected(cyd_state.waste, "", TANK_ROLE_WASTE);
            stored_waste_id = "";
        }
        persist_assignment(stored_fresh_id, stored_waste_id);
        state_dirty = true;
        return;
    }

    ControllerRecord *rec = find_controller(target_id);
    IPAddress target_ip = IPAddress(255, 255, 255, 255);
    if (rec && rec->ip != IPAddress()) target_ip = rec->ip;

    WiFiUDP sender;
    if (!sender.begin(0)) return;
    StaticJsonDocument<256> doc;
    doc["type"] = "cyd_pair";
    doc["event"] = "unpair_request";
    doc["id"] = target_id;
    doc["role"] = role_to_string(role);
    if (cyd_state.diag_id[0]) doc["cyd_id"] = cyd_state.diag_id;
    char payload[256];
    size_t written = serializeJson(doc, payload, sizeof(payload));
    if (written > 0) {
        sender.beginPacket(target_ip, kControllerPort);
        sender.write(reinterpret_cast<const uint8_t *>(payload), written);
        sender.endPacket();
        Serial.printf("[unpair] sent unpair_request to %s role=%s\n", target_id.c_str(), role_to_string(role));
        unpair_pending.active = true;
        unpair_pending.role = role;
        unpair_pending.controller_id = target_id;
        unpair_pending.timeout_ms = millis() + 8000;
    }
    sender.stop();
}

void controller_link_init(Preferences *prefs) {
    prefs_handle = prefs;
    load_assignments_from_nvs();
    load_configs_from_nvs();
    apply_config_to_state(cyd_state.fresh, fresh_config, TANK_ROLE_FRESH);
    apply_config_to_state(cyd_state.waste, waste_config, TANK_ROLE_WASTE);
    state_dirty = true;
}

void controller_link_on_wifi_connected() {
    start_udp();
}

void controller_link_on_wifi_disconnected() {
    stop_udp();
    controllers.clear();
    mark_tank_disconnected(cyd_state.fresh, stored_fresh_id, TANK_ROLE_FRESH);
    mark_tank_disconnected(cyd_state.waste, stored_waste_id, TANK_ROLE_WASTE);
    state_dirty = true;
}

void controller_link_loop() {
    if (!udp_started) {
        apply_to_ui_if_needed();
        return;
    }

    int packet_size = udp.parsePacket();
    while (packet_size > 0) {
        if (packet_size >= static_cast<int>(kMaxPacketSize)) {
            // Drop oversize packets to avoid overflow.
            Serial.printf("[ctrl] dropping oversize UDP packet (%d bytes >= %u)\n", packet_size,
                          static_cast<unsigned>(kMaxPacketSize));
            udp.flush();
            packet_size = udp.parsePacket();
            continue;
        }
        char buf[kMaxPacketSize + 1] = {0};
        const int len = udp.read(buf, static_cast<size_t>(kMaxPacketSize));
        if (len > 0) {
            // If the packet was larger than our buffer, it was truncated.
            if (len >= static_cast<int>(kMaxPacketSize)) {
                Serial.printf("[ctrl] UDP packet truncated (len=%d >= buf=%u)\n", len,
                              static_cast<unsigned>(kMaxPacketSize));
                udp.flush();
                packet_size = udp.parsePacket();
                continue;
            }
            buf[len] = '\0';
            handle_packet(buf, static_cast<size_t>(len), udp.remoteIP());
        }
        packet_size = udp.parsePacket();
    }

    check_for_stale_controllers();
    // Handle unpair timeout
    if (unpair_pending.active && millis() > unpair_pending.timeout_ms) {
        Serial.printf("[unpair] timeout waiting for unpair_ack from %s; clearing locally\n",
                      unpair_pending.controller_id.c_str());
        controller_link_handle_unpair_ack(unpair_pending.controller_id);
        unpair_pending.active = false;
    }

    // Timeout pairing adverts so non-pairing controllers drop from pairing list,
    // and mark controllers offline when state times out.
    const uint32_t now = millis();
    for (auto &rec : controllers) {
        if (rec.pairing_active && rec.last_advert_ts > 0 && now - rec.last_advert_ts > kPairingAdvertTimeoutMs) {
            rec.pairing_active = false;
            Serial.printf("[pair] id=%s pairing advert timeout; pairing_active=false\n", rec.id.c_str());
        }
        if (rec.online && rec.last_state_ts > 0 && now - rec.last_state_ts > kControllerStateTimeoutMs) {
            rec.online = false;
            Serial.printf("[ctrl] id=%s state timeout; marking offline\n", rec.id.c_str());
        }
    }

    // Offline detection for assigned controllers based on state timeout.
    for (auto &rec : controllers) {
        if (rec.online && rec.last_state_ts > 0 && now - rec.last_state_ts > kControllerStateTimeoutMs) {
            rec.online = false;
            Serial.printf("[ctrl] id=%s state timeout; marking offline\n", rec.id.c_str());
        }
    }

    if (!stored_fresh_id.isEmpty()) {
        ControllerRecord *fresh = find_controller(stored_fresh_id);
        if (fresh && !fresh->online && cyd_state.fresh.paired) {
            mark_tank_offline(cyd_state.fresh, stored_fresh_id, TANK_ROLE_FRESH);
            Serial.println("[tank] Fresh marked offline (fault 9)");
            state_dirty = true;
        }
    }
    if (!stored_waste_id.isEmpty()) {
        ControllerRecord *waste = find_controller(stored_waste_id);
        if (waste && !waste->online && cyd_state.waste.paired) {
            mark_tank_offline(cyd_state.waste, stored_waste_id, TANK_ROLE_WASTE);
            Serial.println("[tank] Waste marked offline (fault 9)");
            state_dirty = true;
        }
    }

    apply_to_ui_if_needed();
}
