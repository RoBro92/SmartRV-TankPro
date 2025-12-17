#ifndef CONTROLLER_LINK_H
#define CONTROLLER_LINK_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
#include "cyd_state.h"
#ifdef __cplusplus
}
#endif

// Minimal descriptor for listing discovered controllers in the UI.
struct ControllerSummary {
    String id;
    tank_role_t reported_role;
    bool paired_flag;
    int16_t wifi_signal_dbm;
    uint32_t last_seen_ms;
};

// Initialize controller linkage; call after prefs.begin().
void controller_link_init(Preferences *prefs);
// Start/stop the UDP listener when Wi-Fi connects/disconnects.
void controller_link_on_wifi_connected();
void controller_link_on_wifi_disconnected();
// Process incoming controller packets and update cyd_state.
void controller_link_loop();
// Snapshot of currently seen controllers (recently heard via UDP).
std::vector<ControllerSummary> controller_link_get_controllers();
// Persistently assign a controller id to a role (updates in-memory mapping).
bool controller_link_assign_controller(const String &id, tank_role_t role);
// Send a pairing request to a controller so it can set its role/paired flag.
void controller_link_send_pair_request(const String &id, tank_role_t role, const String &cyd_id);
// Request that the assigned controller for a role unpairs; clears locally on ack/timeout.
void controller_link_request_unpair(tank_role_t role);

struct ControllerConfigUpdate {
    bool has_fill_stop = false;
    uint8_t fill_stop_percent = 0;
    bool has_drain_stop = false;
    uint8_t drain_stop_percent = 0;
    bool has_freeze_enabled = false;
    bool freeze_enabled = false;
    bool has_freeze_threshold = false;
    float freeze_threshold_c = 0.0f;
    bool has_drain_timeout = false;
    uint32_t drain_timeout_ms = 0;
    bool has_empty_volts = false;
    float empty_volts = 0.0f;
    bool has_full_volts = false;
    float full_volts = 0.0f;
    bool has_safety_override = false;
    bool safety_override = false;
    bool has_valve_override = false;
    bool valve_override = false;
};

// Send config updates to the assigned controller for a given role.
void controller_link_send_config_update(tank_role_t role, const ControllerConfigUpdate &cfg);
// Apply a local config change (updates CYD config, persists, updates UI, optionally sends to controller).
void controller_link_apply_local_config(tank_role_t role, const ControllerConfigUpdate &cfg, bool send_to_controller);
// Send a command to the assigned controller (e.g., fill or drain).
void controller_link_send_command(tank_role_t role, const char *action);
void controller_link_send_calibrate_empty(tank_role_t role);
void controller_link_send_calibrate_full(tank_role_t role);
void controller_link_send_restart(tank_role_t role);

#endif  // CONTROLLER_LINK_H
