#include <lvgl.h>
#include <WiFi.h>
#include <Arduino.h>
#include <limits.h>
#include <cstdio>
#include <string>
#include <vector>
#include "ui.h"
#include "cyd_state.h"
#include "controller_link.h"

// Onboarding control (defined in main.cpp)
void start_wifi_onboarding();
void stop_wifi_onboarding();
void factory_reset_and_reboot();

// Keep navigation/overlay logic here so regenerating SquareLine files won't wipe it.
// To add a new navigation:
// 1) Write a small handler that calls _ui_screen_change(target_screen, animation, time_ms, delay_ms, target_init_fn).
// 2) Register it on the source widget(s) in ui_register_custom_actions().

// Navigation targets

// Pairing overlay context
enum class PairingOrigin {
    NONE,
    BOOT,
    HOME,
};

struct PairingContext {
    PairingOrigin origin = PairingOrigin::NONE;
    tank_role_t selected_role = TANK_ROLE_NONE;
    String selected_controller_id;
    tank_role_t role_hint = TANK_ROLE_NONE;
    std::vector<ControllerSummary> controllers;
};

static PairingContext pairing_ctx;

static const char *role_label(tank_role_t role) {
    switch (role) {
        case TANK_ROLE_FRESH: return "Fresh";
        case TANK_ROLE_WASTE: return "Waste";
        default: return "Unassigned";
    }
}

static void update_confirm_enabled() {
    const bool ready = pairing_ctx.selected_controller_id.length() > 0 && pairing_ctx.selected_role != TANK_ROLE_NONE;
    if (ui_bootAssignConfirmButton) {
        if (ready && pairing_ctx.origin == PairingOrigin::BOOT) {
            lv_obj_clear_state(ui_bootAssignConfirmButton, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(ui_bootAssignConfirmButton, LV_STATE_DISABLED);
        }
    }
    if (ui_homeAssignConfirmButton) {
        if (ready && pairing_ctx.origin == PairingOrigin::HOME) {
            lv_obj_clear_state(ui_homeAssignConfirmButton, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(ui_homeAssignConfirmButton, LV_STATE_DISABLED);
        }
    }
}

static void set_role_button_state(lv_obj_t *fresh_btn, lv_obj_t *waste_btn, tank_role_t selected) {
    if (fresh_btn) {
        const bool on = selected == TANK_ROLE_FRESH;
        lv_obj_set_style_bg_color(fresh_btn, lv_color_hex(on ? 0x10E857 : 0x3A3A3A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(fresh_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (waste_btn) {
        const bool on = selected == TANK_ROLE_WASTE;
        lv_obj_set_style_bg_color(waste_btn, lv_color_hex(on ? 0x10E857 : 0x3A3A3A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(waste_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void pairing_ui_clear() {
    pairing_ctx = PairingContext();
    set_role_button_state(ui_bootAssignFreshButton, ui_bootAssignWasteButton, TANK_ROLE_NONE);
    set_role_button_state(ui_homeAssignFreshButton, ui_homeAssignWasteButton, TANK_ROLE_NONE);
    if (ui_bootAssignDeviceList) lv_dropdown_set_options(ui_bootAssignDeviceList, "");
    if (ui_homeAssignDeviceList) lv_dropdown_set_options(ui_homeAssignDeviceList, "");
    update_confirm_enabled();
}

static void pairing_ui_begin(PairingOrigin origin, tank_role_t hint_role) {
    pairing_ctx.origin = origin;
    pairing_ctx.selected_role = hint_role;
    pairing_ctx.role_hint = hint_role;
    pairing_ctx.selected_controller_id = "";
    pairing_ctx.controllers.clear();
    if (origin == PairingOrigin::BOOT) {
        set_role_button_state(ui_bootAssignFreshButton, ui_bootAssignWasteButton, pairing_ctx.selected_role);
    } else if (origin == PairingOrigin::HOME) {
        set_role_button_state(ui_homeAssignFreshButton, ui_homeAssignWasteButton, pairing_ctx.selected_role);
    }
    update_confirm_enabled();
}

static void apply_selection_from_dropdown(lv_obj_t *dd) {
    pairing_ctx.selected_controller_id = "";
    if (!dd || pairing_ctx.controllers.empty()) {
        update_confirm_enabled();
        return;
    }
    uint16_t sel = lv_dropdown_get_selected(dd);
    if (sel < pairing_ctx.controllers.size()) {
        pairing_ctx.selected_controller_id = pairing_ctx.controllers[sel].id;
    }
    update_confirm_enabled();
}

static size_t populate_assign_dropdown(lv_obj_t *dd) {
    pairing_ctx.controllers = controller_link_get_controllers();
    Serial.printf("[pair] discovered %u controllers\n", static_cast<unsigned>(pairing_ctx.controllers.size()));
    std::string options;
    for (size_t i = 0; i < pairing_ctx.controllers.size(); ++i) {
        const auto &c = pairing_ctx.controllers[i];
        options += c.id.c_str();
        options += " (";
        options += role_label(c.reported_role);
        if (c.paired_flag) options += ", paired";
        options += ")";
        if (i + 1 < pairing_ctx.controllers.size()) options += "\n";
    }
    if (options.empty()) {
        options = "No devices found";
        lv_dropdown_set_options(dd, options.c_str());
        pairing_ctx.selected_controller_id = "";
        update_confirm_enabled();
        return pairing_ctx.controllers.size();
    }
    lv_dropdown_set_options(dd, options.c_str());
    lv_dropdown_set_selected(dd, 0);
    if (!pairing_ctx.controllers.empty()) {
        pairing_ctx.selected_controller_id = pairing_ctx.controllers[0].id;
    }
    update_confirm_enabled();
    return pairing_ctx.controllers.size();
}

static void hide_boot_assign_overlay() {
    if (ui_bootOverlayAssignRole) {
        _ui_opacity_set(ui_bootOverlayAssignRole, 0);
        lv_obj_add_flag(ui_bootOverlayAssignRole, LV_OBJ_FLAG_HIDDEN);
    }
    pairing_ui_clear();
}

static void hide_home_assign_overlay() {
    if (ui_hootOverlayAssignRole) {
        _ui_opacity_set(ui_hootOverlayAssignRole, 0);
        lv_obj_add_flag(ui_hootOverlayAssignRole, LV_OBJ_FLAG_HIDDEN);
    }
    pairing_ui_clear();
}

void pairing_ui_on_pair_ack(const String &controller_id, tank_role_t role) {
    // Close any active overlay if the ack matches the current selection (or no selection yet).
    if (pairing_ctx.origin == PairingOrigin::NONE) return;
    if (pairing_ctx.selected_controller_id.length() > 0 &&
        pairing_ctx.selected_controller_id != controller_id) {
        return;
    }
    if (pairing_ctx.selected_role != TANK_ROLE_NONE && pairing_ctx.selected_role != role) {
        return;
    }
    if (pairing_ctx.origin == PairingOrigin::BOOT) {
        hide_boot_assign_overlay();
    } else if (pairing_ctx.origin == PairingOrigin::HOME) {
        hide_home_assign_overlay();
    }
    cyd_state_apply_to_home_screen();
}

void pairing_ui_on_unpair_complete(tank_role_t role) {
    LV_UNUSED(role);
    cyd_state_apply_to_home_screen();
    cyd_state_apply_to_fresh_screen();
    cyd_state_apply_to_waste_screen();
    cyd_state_apply_to_freshsettings_screen();
    cyd_state_apply_to_wastesettings_screen();
    _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

void cyd_ui_set_theme_selector(int idx) {
    if (!ui_cydTheme) return;
    lv_dropdown_set_selected(ui_cydTheme, idx);
}

// Timed scan refresher for overlays
static lv_timer_t *pairing_scan_timer = nullptr;
static uint32_t pairing_scan_end_ms = 0;

static void pairing_scan_tick(lv_timer_t *t) {
    LV_UNUSED(t);
    if (millis() > pairing_scan_end_ms) {
        if (pairing_scan_timer) {
            lv_timer_del(pairing_scan_timer);
            pairing_scan_timer = nullptr;
        }
        Serial.printf("[pair] scan finished, %u controllers\n", static_cast<unsigned>(pairing_ctx.controllers.size()));
        return;
    }
    if (pairing_ctx.origin == PairingOrigin::BOOT) {
        populate_assign_dropdown(ui_bootAssignDeviceList);
    } else if (pairing_ctx.origin == PairingOrigin::HOME) {
        populate_assign_dropdown(ui_homeAssignDeviceList);
    }
}

static void start_pairing_scan() {
    pairing_scan_end_ms = millis() + 10000;
    if (pairing_scan_timer) {
        lv_timer_reset(pairing_scan_timer);
    } else {
        pairing_scan_timer = lv_timer_create(pairing_scan_tick, 1000, nullptr);
    }
}

static void show_boot_assign_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_bootOverlayAssignRole) return;
    pairing_ui_begin(PairingOrigin::BOOT, TANK_ROLE_NONE);
    lv_obj_add_flag(ui_bootOverlayAssignRole, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_bootOverlayAssignRole, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_bootOverlayAssignRole, 255);
    lv_obj_move_foreground(ui_bootOverlayAssignRole);
    populate_assign_dropdown(ui_bootAssignDeviceList);
    start_pairing_scan();
    set_role_button_state(ui_bootAssignFreshButton, ui_bootAssignWasteButton, pairing_ctx.selected_role);
    update_confirm_enabled();
}

static void show_home_assign_overlay(tank_role_t hint) {
    if (!ui_hootOverlayAssignRole) return;
    pairing_ui_begin(PairingOrigin::HOME, hint);
    lv_obj_add_flag(ui_hootOverlayAssignRole, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_hootOverlayAssignRole, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_hootOverlayAssignRole, 255);
    lv_obj_move_foreground(ui_hootOverlayAssignRole);
    populate_assign_dropdown(ui_homeAssignDeviceList);
    start_pairing_scan();
    set_role_button_state(ui_homeAssignFreshButton, ui_homeAssignWasteButton, pairing_ctx.selected_role);
    update_confirm_enabled();
}

static void home_open_assign_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    show_home_assign_overlay(TANK_ROLE_FRESH);
}

static void home_open_assign_waste(lv_event_t *e) {
    LV_UNUSED(e);
    show_home_assign_overlay(TANK_ROLE_WASTE);
}

static void assign_role_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    pairing_ctx.selected_role = TANK_ROLE_FRESH;
    set_role_button_state(ui_bootAssignFreshButton, ui_bootAssignWasteButton, pairing_ctx.origin == PairingOrigin::BOOT ? pairing_ctx.selected_role : TANK_ROLE_NONE);
    set_role_button_state(ui_homeAssignFreshButton, ui_homeAssignWasteButton, pairing_ctx.origin == PairingOrigin::HOME ? pairing_ctx.selected_role : TANK_ROLE_NONE);
    update_confirm_enabled();
}

static void assign_role_waste(lv_event_t *e) {
    LV_UNUSED(e);
    pairing_ctx.selected_role = TANK_ROLE_WASTE;
    set_role_button_state(ui_bootAssignFreshButton, ui_bootAssignWasteButton, pairing_ctx.origin == PairingOrigin::BOOT ? pairing_ctx.selected_role : TANK_ROLE_NONE);
    set_role_button_state(ui_homeAssignFreshButton, ui_homeAssignWasteButton, pairing_ctx.origin == PairingOrigin::HOME ? pairing_ctx.selected_role : TANK_ROLE_NONE);
    update_confirm_enabled();
}

static void boot_assign_dropdown_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (pairing_ctx.origin != PairingOrigin::BOOT) return;
    apply_selection_from_dropdown(ui_bootAssignDeviceList);
}

static void home_assign_dropdown_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (pairing_ctx.origin != PairingOrigin::HOME) return;
    apply_selection_from_dropdown(ui_homeAssignDeviceList);
}

static void boot_assign_cancel(lv_event_t *e) {
    LV_UNUSED(e);
    hide_boot_assign_overlay();
}

static void home_assign_cancel(lv_event_t *e) {
    LV_UNUSED(e);
    hide_home_assign_overlay();
}

static void boot_assign_confirm(lv_event_t *e) {
    LV_UNUSED(e);
    if (pairing_ctx.origin != PairingOrigin::BOOT) return;
    if (pairing_ctx.selected_controller_id.isEmpty() || pairing_ctx.selected_role == TANK_ROLE_NONE) return;
    if (controller_link_assign_controller(pairing_ctx.selected_controller_id, pairing_ctx.selected_role)) {
        controller_link_send_pair_request(pairing_ctx.selected_controller_id, pairing_ctx.selected_role, String(cyd_state.diag_id));
        hide_boot_assign_overlay();
        cyd_state_apply_to_home_screen();
    }
}

static void home_assign_confirm(lv_event_t *e) {
    LV_UNUSED(e);
    if (pairing_ctx.origin != PairingOrigin::HOME) return;
    if (pairing_ctx.selected_controller_id.isEmpty() || pairing_ctx.selected_role == TANK_ROLE_NONE) return;
    if (controller_link_assign_controller(pairing_ctx.selected_controller_id, pairing_ctx.selected_role)) {
        controller_link_send_pair_request(pairing_ctx.selected_controller_id, pairing_ctx.selected_role, String(cyd_state.diag_id));
        hide_home_assign_overlay();
        cyd_state_apply_to_home_screen();
    }
}

// --- Boot ---
static void show_boot_wifi(lv_event_t *e) {
    LV_UNUSED(e);
    start_wifi_onboarding();
    if (ui_overlayBootWifi == NULL) return;
    lv_obj_clear_flag(ui_overlayBootWifi, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_overlayBootWifi, 255);
    lv_obj_move_foreground(ui_overlayBootWifi);
}

static void hide_boot_wifi(lv_event_t *e) {
    LV_UNUSED(e);
    stop_wifi_onboarding();
    if (ui_overlayBootWifi == NULL) return;
    _ui_opacity_set(ui_overlayBootWifi, 0);
    lv_obj_add_flag(ui_overlayBootWifi, LV_OBJ_FLAG_HIDDEN);
}

static void show_boot_direct(lv_event_t *e) {
    LV_UNUSED(e);
    // Repurpose the Direct button to start controller assignment.
    show_boot_assign_overlay(e);
}

static void hide_boot_direct(lv_event_t *e) {
    LV_UNUSED(e);
    if (ui_overlayBootDirect == NULL) return;
    _ui_opacity_set(ui_overlayBootDirect, 0);
    lv_obj_add_flag(ui_overlayBootDirect, LV_OBJ_FLAG_HIDDEN);
}

// --- Home ---

static void home_to_settings(lv_event_t *e) {
    LV_UNUSED(e);
    printf("[nav] home_to_settings\n");
    _ui_screen_change(&ui_cydsettings, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_cydsettings_screen();
}

static void home_to_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_fresh, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_fresh_screen();
}

static void home_to_waste(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_waste, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_waste_screen();
}
// --- Cyd Settings ---

static void cydsettings_to_home(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

// Cyd Settings overlay
static void cydsettings_show_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    const bool wifi_connected = WiFi.status() == WL_CONNECTED;
    if (wifi_connected) {
        const String ip = WiFi.localIP().toString();
        const String mac = WiFi.macAddress();
        snprintf(cyd_state.diag_ip, sizeof(cyd_state.diag_ip), "%s", ip.c_str());
        snprintf(cyd_state.diag_mac, sizeof(cyd_state.diag_mac), "%s", mac.c_str());
        snprintf(cyd_state.diag_status, sizeof(cyd_state.diag_status), "Connected");
        cyd_state.diag_signal_dbm = WiFi.RSSI();
    } else {
        cyd_state.diag_ip[0] = '\0';
        cyd_state.diag_mac[0] = '\0';
        snprintf(cyd_state.diag_status, sizeof(cyd_state.diag_status), "Not connected");
        cyd_state.diag_signal_dbm = INT16_MIN;
    }
    cyd_state.diag_uptime_s = millis() / 1000;
    cyd_state_apply_to_cydsettings_diag_overlay();
    if (!ui_cydsettingsDiagnosticOverlay) return;
    lv_obj_add_flag(ui_cydsettingsDiagnosticOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_cydsettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_cydsettingsDiagnosticOverlay, 255);
    lv_obj_move_foreground(ui_cydsettingsDiagnosticOverlay);
}

static void cydsettings_hide_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_cydsettingsDiagnosticOverlay) return;
    _ui_opacity_set(ui_cydsettingsDiagnosticOverlay, 0);
    lv_obj_add_flag(ui_cydsettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
}

static void cydsettings_show_factory_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_cydsettingsFactoryResetConfirmationOverlay) return;
    // Expand to cover the screen so underlying widgets cannot be pressed
    lv_obj_set_width(ui_cydsettingsFactoryResetConfirmationOverlay, lv_pct(100));
    lv_obj_set_height(ui_cydsettingsFactoryResetConfirmationOverlay, lv_pct(100));
    lv_obj_set_align(ui_cydsettingsFactoryResetConfirmationOverlay, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_cydsettingsFactoryResetConfirmationOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_cydsettingsFactoryResetConfirmationOverlay, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_cydsettingsFactoryResetConfirmationOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_cydsettingsFactoryResetConfirmationOverlay, 255);
    lv_obj_move_foreground(ui_cydsettingsFactoryResetConfirmationOverlay);
}

static void cydsettings_hide_factory_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_cydsettingsFactoryResetConfirmationOverlay) return;
    _ui_opacity_set(ui_cydsettingsFactoryResetConfirmationOverlay, 0);
    lv_obj_add_flag(ui_cydsettingsFactoryResetConfirmationOverlay, LV_OBJ_FLAG_HIDDEN);
}

static uint32_t cyd_diag_press_start_ms = 0;
static bool cyd_diag_longpress_consumed = false;
static void cydsettings_diag_button_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        cyd_diag_press_start_ms = lv_tick_get();
        cyd_diag_longpress_consumed = false;
    } else if (code == LV_EVENT_PRESSING) {
        if (cyd_diag_press_start_ms != 0 && lv_tick_elaps(cyd_diag_press_start_ms) >= 5000) {
            cyd_diag_press_start_ms = 0;
            cyd_diag_longpress_consumed = true;
            cydsettings_show_factory_overlay(nullptr);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        cyd_diag_press_start_ms = 0;
        if (cyd_diag_longpress_consumed) {
            // Swallow the click after a long press
            return;
        }
    } else if (code == LV_EVENT_CLICKED) {
        if (cyd_diag_longpress_consumed) return;
        cydsettings_show_diag_overlay(e);
    }
}

static void cydsettings_factory_reset_confirm(lv_event_t *e) {
    LV_UNUSED(e);
    cydsettings_hide_factory_overlay(nullptr);
    factory_reset_and_reboot();
}

// --- Boot -> Home ---
static void boot_to_home(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

// --- Fresh ---

static void fresh_to_home(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

static void fresh_to_freshfaults(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_freshfaults, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_freshfaults_screen();
}

static void fresh_clear_faults(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_command(TANK_ROLE_FRESH, "clear_faults");
}

static void fresh_start_fill(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_command(TANK_ROLE_FRESH, "fill");
}

static void fresh_start_drain(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_command(TANK_ROLE_FRESH, "drain");
}

static void fresh_to_freshsettings(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_freshsettings, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_freshsettings_screen();
}

// --- Fresh Settings ---
static void freshsettings_to_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_fresh, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

// Fresh Settings overlays
static void freshsettings_show_fill_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsFillOverlay) return;
    lv_obj_clear_flag(ui_freshsettingsFillOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_freshsettingsFillOverlay, 255);
    lv_obj_move_foreground(ui_freshsettingsFillOverlay);
}

static void freshsettings_hide_fill_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsFillOverlay) return;
    _ui_opacity_set(ui_freshsettingsFillOverlay, 0);
    lv_obj_add_flag(ui_freshsettingsFillOverlay, LV_OBJ_FLAG_HIDDEN);
    ControllerConfigUpdate cfg;
    cfg.has_fill_stop = true;
    cfg.fill_stop_percent = cyd_state.fresh.stop_level_percent;
    Serial.printf("[cfg] UI commit role=Fresh field=fill_stop_percent new=%u at t=%lums\n",
                  cfg.fill_stop_percent, static_cast<unsigned long>(millis()));
    controller_link_apply_local_config(TANK_ROLE_FRESH, cfg, true);
}

static void freshsettings_show_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsDiagnosticOverlay) return;
    lv_obj_add_flag(ui_freshsettingsDiagnosticOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_freshsettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_freshsettingsDiagnosticOverlay, 255);
    lv_obj_move_foreground(ui_freshsettingsDiagnosticOverlay);
    cyd_state_apply_to_freshsettings_diag_overlay();
}

static void freshsettings_hide_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsDiagnosticOverlay) return;
    _ui_opacity_set(ui_freshsettingsDiagnosticOverlay, 0);
    lv_obj_add_flag(ui_freshsettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
}

static void freshsettings_slider_changed(lv_event_t *e) {
    if (!ui_freshsettingsOverlayFillSlider) return;
    const int32_t v = lv_slider_get_value(ui_freshsettingsOverlayFillSlider);
    cyd_state.fresh.stop_level_percent = v < 0 ? 0 : (v > 100 ? 100 : static_cast<uint8_t>(v));
    if (ui_freshsettingsOverlayFillPercentage) {
        lv_label_set_text_fmt(ui_freshsettingsOverlayFillPercentage, "%u%%", cyd_state.fresh.stop_level_percent);
    }
    if (ui_freshsettingsFillStopLevelLabel) {
        lv_label_set_text_fmt(ui_freshsettingsFillStopLevelLabel, "%u%%", cyd_state.fresh.stop_level_percent);
    }
}

static void freshsettings_freeze_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsFreezeProtection) return;
    uint16_t sel = lv_dropdown_get_selected(ui_freshsettingsFreezeProtection);
    if (sel > 5) sel = 0;
    cyd_state.fresh.freeze_setting = static_cast<uint8_t>(sel);
    cyd_state.fresh.freeze_enabled = (sel > 0);
    ControllerConfigUpdate cfg;
    cfg.has_freeze_enabled = true;
    cfg.freeze_enabled = cyd_state.fresh.freeze_enabled;
    cfg.has_freeze_threshold = true;
    cfg.freeze_threshold_c = static_cast<float>(sel);
    Serial.printf("[cfg] UI change role=Fresh field=freeze_enabled/thresh new_en=%s new_thresh=%.2f at t=%lums\n",
                  cfg.freeze_enabled ? "true" : "false", cfg.freeze_threshold_c,
                  static_cast<unsigned long>(millis()));
    controller_link_apply_local_config(TANK_ROLE_FRESH, cfg, true);
}

static void freshsettings_safety_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsSafetyOveride) return;
    cyd_state.fresh.safety_override_enabled = lv_obj_has_state(ui_freshsettingsSafetyOveride, LV_STATE_CHECKED);
    ControllerConfigUpdate cfg;
    cfg.has_safety_override = true;
    cfg.safety_override = cyd_state.fresh.safety_override_enabled;
    controller_link_apply_local_config(TANK_ROLE_FRESH, cfg, true);
}

static void freshsettings_valve_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsValveOveride) return;
    cyd_state.fresh.valve_override_enabled = lv_obj_has_state(ui_freshsettingsValveOveride, LV_STATE_CHECKED);
    ControllerConfigUpdate cfg;
    cfg.has_valve_override = true;
    cfg.valve_override = cyd_state.fresh.valve_override_enabled;
    controller_link_apply_local_config(TANK_ROLE_FRESH, cfg, true);
}

static void freshsettings_restart(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_restart(TANK_ROLE_FRESH);
}

static uint32_t fresh_remove_press_ms = 0;
static bool fresh_remove_ready = false;
static bool fresh_remove_color_init = false;
static lv_color_t fresh_remove_base_color;
static void freshsettings_remove(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (!fresh_remove_color_init && ui_freshsettingsRemove) {
        fresh_remove_base_color = lv_obj_get_style_bg_color(ui_freshsettingsRemove, LV_PART_MAIN);
        fresh_remove_color_init = true;
    }
    if (code == LV_EVENT_PRESSED) {
        fresh_remove_press_ms = lv_tick_get();
        fresh_remove_ready = false;
    } else if (code == LV_EVENT_PRESSING && fresh_remove_press_ms != 0) {
        const uint32_t elapsed = lv_tick_elaps(fresh_remove_press_ms);
        if (!fresh_remove_ready && elapsed >= 3000) {
            fresh_remove_ready = true;
            if (ui_freshsettingsRemove) {
                lv_obj_set_style_bg_color(ui_freshsettingsRemove, lv_color_hex(0x10E857),
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (fresh_remove_press_ms == 0) return;
        const uint32_t elapsed = lv_tick_elaps(fresh_remove_press_ms);
        fresh_remove_press_ms = 0;
        if (elapsed >= 3000) {
            controller_link_request_unpair(TANK_ROLE_FRESH);
        } else {
            Serial.println("[unpair] hold Fresh Remove for 3s to confirm");
        }
        fresh_remove_ready = false;
        if (ui_freshsettingsRemove && fresh_remove_color_init) {
            lv_obj_set_style_bg_color(ui_freshsettingsRemove, fresh_remove_base_color,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void freshsettings_set_full(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_calibrate_full(TANK_ROLE_FRESH);
}

static void freshsettings_set_empty(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_calibrate_empty(TANK_ROLE_FRESH);
}

// --- Fresh Faults ---
static void freshfaults_to_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_fresh, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}


// --- Waste  ---
static void waste_to_home(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

static void waste_to_wastefaults(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_wastefaults, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_wastefaults_screen();
}

static void waste_clear_faults(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_command(TANK_ROLE_WASTE, "clear_faults");
}

static void waste_to_wastesettings(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_wastesettings, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
    cyd_state_apply_to_wastesettings_screen();
}

// --- Waste Settings ---
static void wastesettings_to_waste(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_waste, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

// Waste Settings overlays
static void wastesettings_show_drain_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsDrainOverlay) return;
    lv_obj_clear_flag(ui_wastesettingsDrainOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_wastesettingsDrainOverlay, 255);
    lv_obj_move_foreground(ui_wastesettingsDrainOverlay);
}

static void wastesettings_hide_drain_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsDrainOverlay) return;
    _ui_opacity_set(ui_wastesettingsDrainOverlay, 0);
    lv_obj_add_flag(ui_wastesettingsDrainOverlay, LV_OBJ_FLAG_HIDDEN);
    ControllerConfigUpdate cfg;
    cfg.has_drain_stop = true;
    cfg.drain_stop_percent = cyd_state.waste.stop_level_percent;
    Serial.printf("[cfg] UI commit role=Waste field=drain_stop_percent new=%u at t=%lums\n",
                  cfg.drain_stop_percent, static_cast<unsigned long>(millis()));
    controller_link_apply_local_config(TANK_ROLE_WASTE, cfg, true);
}

static void wastesettings_show_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsDiagnosticOverlay) return;
    lv_obj_add_flag(ui_wastesettingsDiagnosticOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_wastesettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_wastesettingsDiagnosticOverlay, 255);
    lv_obj_move_foreground(ui_wastesettingsDiagnosticOverlay);
    cyd_state_apply_to_wastesettings_diag_overlay();
}

static void wastesettings_hide_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsDiagnosticOverlay) return;
    _ui_opacity_set(ui_wastesettingsDiagnosticOverlay, 0);
    lv_obj_add_flag(ui_wastesettingsDiagnosticOverlay, LV_OBJ_FLAG_HIDDEN);
}

static void wastesettings_slider_changed(lv_event_t *e) {
    if (!ui_wastesettingsOverlayDrainSlider) return;
    const int32_t v = lv_slider_get_value(ui_wastesettingsOverlayDrainSlider);
    cyd_state.waste.stop_level_percent = v < 0 ? 0 : (v > 100 ? 100 : static_cast<uint8_t>(v));
    if (ui_wastesettingsOverlayDrainPercentage) {
        lv_label_set_text_fmt(ui_wastesettingsOverlayDrainPercentage, "%u%%", cyd_state.waste.stop_level_percent);
    }
    if (ui_wastesettingsDrainStopLevelLabel) {
        lv_label_set_text_fmt(ui_wastesettingsDrainStopLevelLabel, "%u%%", cyd_state.waste.stop_level_percent);
    }
}

static void wastesettings_freeze_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsFreezeProtection) return;
    uint16_t sel = lv_dropdown_get_selected(ui_wastesettingsFreezeProtection);
    if (sel > 5) sel = 0;
    cyd_state.waste.freeze_setting = static_cast<uint8_t>(sel);
    cyd_state.waste.freeze_enabled = (sel > 0);
    ControllerConfigUpdate cfg;
    cfg.has_freeze_enabled = true;
    cfg.freeze_enabled = cyd_state.waste.freeze_enabled;
    cfg.has_freeze_threshold = true;
    cfg.freeze_threshold_c = static_cast<float>(sel);
    Serial.printf("[cfg] UI change role=Waste field=freeze_enabled/thresh new_en=%s new_thresh=%.2f at t=%lums\n",
                  cfg.freeze_enabled ? "true" : "false", cfg.freeze_threshold_c,
                  static_cast<unsigned long>(millis()));
    controller_link_apply_local_config(TANK_ROLE_WASTE, cfg, true);
}

static void wastesettings_safety_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsSafetyOveride) return;
    cyd_state.waste.safety_override_enabled = lv_obj_has_state(ui_wastesettingsSafetyOveride, LV_STATE_CHECKED);
    ControllerConfigUpdate cfg;
    cfg.has_safety_override = true;
    cfg.safety_override = cyd_state.waste.safety_override_enabled;
    controller_link_apply_local_config(TANK_ROLE_WASTE, cfg, true);
}

static void wastesettings_valve_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsValveOveride) return;
    cyd_state.waste.valve_override_enabled = lv_obj_has_state(ui_wastesettingsValveOveride, LV_STATE_CHECKED);
    ControllerConfigUpdate cfg;
    cfg.has_valve_override = true;
    cfg.valve_override = cyd_state.waste.valve_override_enabled;
    controller_link_apply_local_config(TANK_ROLE_WASTE, cfg, true);
}

static void wastesettings_restart(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_restart(TANK_ROLE_WASTE);
}

static uint32_t waste_remove_press_ms = 0;
static bool waste_remove_ready = false;
static bool waste_remove_color_init = false;
static lv_color_t waste_remove_base_color;
static void wastesettings_remove(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (!waste_remove_color_init && ui_wastesettingsRemove) {
        waste_remove_base_color = lv_obj_get_style_bg_color(ui_wastesettingsRemove, LV_PART_MAIN);
        waste_remove_color_init = true;
    }
    if (code == LV_EVENT_PRESSED) {
        waste_remove_press_ms = lv_tick_get();
        waste_remove_ready = false;
    } else if (code == LV_EVENT_PRESSING && waste_remove_press_ms != 0) {
        const uint32_t elapsed = lv_tick_elaps(waste_remove_press_ms);
        if (!waste_remove_ready && elapsed >= 3000) {
            waste_remove_ready = true;
            if (ui_wastesettingsRemove) {
                lv_obj_set_style_bg_color(ui_wastesettingsRemove, lv_color_hex(0x10E857),
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (waste_remove_press_ms == 0) return;
        const uint32_t elapsed = lv_tick_elaps(waste_remove_press_ms);
        waste_remove_press_ms = 0;
        if (elapsed >= 3000) {
            controller_link_request_unpair(TANK_ROLE_WASTE);
        } else {
            Serial.println("[unpair] hold Waste Remove for 3s to confirm");
        }
        waste_remove_ready = false;
        if (ui_wastesettingsRemove && waste_remove_color_init) {
            lv_obj_set_style_bg_color(ui_wastesettingsRemove, waste_remove_base_color,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void wastesettings_set_full(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_calibrate_full(TANK_ROLE_WASTE);
}

static void wastesettings_set_empty(lv_event_t *e) {
    LV_UNUSED(e);
    controller_link_send_calibrate_empty(TANK_ROLE_WASTE);
}

// --- Waste Faults ---
static void wastefaults_to_waste(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_waste, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
}

// --- Debug: simple level adjustor ---
static void home_debug_adjust(lv_event_t *e) {
    LV_UNUSED(e);
    // Debug disabled in production
}

// Overlay helpers


void ui_register_custom_actions() {
    // --- Boot overlays ---
    if (ui_bootWifiButton) {
        lv_obj_add_event_cb(ui_bootWifiButton, show_boot_wifi, LV_EVENT_CLICKED, NULL);
    }
    if (ui_buttonBootWifiBack) {
        lv_obj_add_event_cb(ui_buttonBootWifiBack, hide_boot_wifi, LV_EVENT_CLICKED, NULL);
    }
    if (ui_bootDirectButton) {
        lv_obj_add_event_cb(ui_bootDirectButton, show_boot_direct, LV_EVENT_CLICKED, NULL);
    }
    if (ui_buttonbootdirectback) {
        lv_obj_add_event_cb(ui_buttonbootdirectback, hide_boot_direct, LV_EVENT_CLICKED, NULL);
    }
    if (ui_buttonbootdirectscan) {
        lv_obj_add_flag(ui_buttonbootdirectscan, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_buttonbootdirectscan, boot_to_home, LV_EVENT_CLICKED, NULL);
    }
    if (ui_bootAssignDeviceList) {
        lv_obj_add_event_cb(ui_bootAssignDeviceList, boot_assign_dropdown_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_bootAssignFreshButton) {
        lv_obj_add_event_cb(ui_bootAssignFreshButton, assign_role_fresh, LV_EVENT_CLICKED, NULL);
    }
    if (ui_bootAssignWasteButton) {
        lv_obj_add_event_cb(ui_bootAssignWasteButton, assign_role_waste, LV_EVENT_CLICKED, NULL);
    }
    if (ui_bootAssignCancelButton) {
        lv_obj_add_event_cb(ui_bootAssignCancelButton, boot_assign_cancel, LV_EVENT_CLICKED, NULL);
    }
    if (ui_bootAssignConfirmButton) {
        lv_obj_add_event_cb(ui_bootAssignConfirmButton, boot_assign_confirm, LV_EVENT_CLICKED, NULL);
    }

    // --- Home  ---
    if (ui_homeCydsettingsButton) {
        lv_obj_add_flag(ui_homeCydsettingsButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_homeCydsettingsButton, home_to_settings, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeFreshButton) {
        lv_obj_add_flag(ui_homeFreshButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_homeFreshButton, home_to_fresh, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeWasteCard) {
        lv_obj_add_flag(ui_homeWasteButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_homeWasteButton, home_to_waste, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeFreshSetupButton) {
        lv_obj_add_event_cb(ui_homeFreshSetupButton, home_open_assign_fresh, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeWasteSetupButton) {
        lv_obj_add_event_cb(ui_homeWasteSetupButton, home_open_assign_waste, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeAssignDeviceList) {
        lv_obj_add_event_cb(ui_homeAssignDeviceList, home_assign_dropdown_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_homeAssignFreshButton) {
        lv_obj_add_event_cb(ui_homeAssignFreshButton, assign_role_fresh, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeAssignWasteButton) {
        lv_obj_add_event_cb(ui_homeAssignWasteButton, assign_role_waste, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeAssignCancelButton) {
        lv_obj_add_event_cb(ui_homeAssignCancelButton, home_assign_cancel, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeAssignConfirmButton) {
        lv_obj_add_event_cb(ui_homeAssignConfirmButton, home_assign_confirm, LV_EVENT_CLICKED, NULL);
    }
    // Debug button: tap to cycle levels for visual testing
    if (ui_homeWasteButton) {
        lv_obj_add_event_cb(ui_homeWasteButton, home_debug_adjust, LV_EVENT_LONG_PRESSED, NULL);
    }

    // --- Cyd Settings ---
    if (ui_cydsettingsBackButton) {
        lv_obj_add_flag(ui_cydsettingsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydsettingsBackButton, cydsettings_to_home, LV_EVENT_CLICKED, NULL);
    }
    if (ui_cydDiagnosticButton) {
        lv_obj_add_flag(ui_cydDiagnosticButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydDiagnosticButton, cydsettings_diag_button_handler, LV_EVENT_ALL, NULL);
    }
    if (ui_cydsettingsResetBackButton) {
        lv_obj_add_flag(ui_cydsettingsResetBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydsettingsResetBackButton, cydsettings_hide_factory_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_cydsettingsResetButton) {
        lv_obj_add_flag(ui_cydsettingsResetButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydsettingsResetButton, cydsettings_factory_reset_confirm, LV_EVENT_CLICKED, NULL);
    }
    if (ui_cydsettingDiagnosticOverlayBackButton) {
        lv_obj_add_flag(ui_cydsettingDiagnosticOverlayBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydsettingDiagnosticOverlayBackButton, cydsettings_hide_diag_overlay, LV_EVENT_CLICKED, NULL);
    }

    // --- Fresh ---    
    if (ui_freshBackButton) {
        lv_obj_add_flag(ui_freshBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshBackButton, fresh_to_home, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshFaultButton) {
        lv_obj_add_flag(ui_freshFaultButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshFaultButton, fresh_to_freshfaults, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshfaultsClearButton) {
        lv_obj_add_flag(ui_freshfaultsClearButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshfaultsClearButton, fresh_clear_faults, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshSettingsButton) {       
        lv_obj_add_flag(ui_freshSettingsButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshSettingsButton, fresh_to_freshsettings, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshFillButton) {
        lv_obj_add_event_cb(ui_freshFillButton, fresh_start_fill, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshDrainButton) {
        lv_obj_add_event_cb(ui_freshDrainButton, fresh_start_drain, LV_EVENT_CLICKED, NULL);
    }
    
    // --- Fresh Settings ---
    if (ui_freshsettingsBackButton) {
        lv_obj_add_flag(ui_freshsettingsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshsettingsBackButton, freshsettings_to_fresh, LV_EVENT_CLICKED, NULL);
    }   
    if (ui_freshsettingsFillLevelButton) {
        lv_obj_add_event_cb(ui_freshsettingsFillLevelButton, freshsettings_show_fill_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsOverlayBackButton) {
        lv_obj_add_event_cb(ui_freshsettingsOverlayBackButton, freshsettings_hide_fill_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsOverlayFillSlider) {
        lv_obj_add_event_cb(ui_freshsettingsOverlayFillSlider, freshsettings_slider_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_freshsettingsDiagnostic) {
        lv_obj_add_event_cb(ui_freshsettingsDiagnostic, freshsettings_show_diag_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingDiagnosticOverlayBackButton) {
        lv_obj_add_event_cb(ui_freshsettingDiagnosticOverlayBackButton, freshsettings_hide_diag_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsRestart) {
        lv_obj_add_event_cb(ui_freshsettingsRestart, freshsettings_restart, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsFreezeProtection) {
        lv_obj_add_event_cb(ui_freshsettingsFreezeProtection, freshsettings_freeze_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_freshsettingsSafetyOveride) {
        lv_obj_add_event_cb(ui_freshsettingsSafetyOveride, freshsettings_safety_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_freshsettingsValveOveride) {
        lv_obj_add_event_cb(ui_freshsettingsValveOveride, freshsettings_valve_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_freshsettingsSetFullButton) {
        lv_obj_add_event_cb(ui_freshsettingsSetFullButton, freshsettings_set_full, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsSetEmptyButton) {
        lv_obj_add_event_cb(ui_freshsettingsSetEmptyButton, freshsettings_set_empty, LV_EVENT_CLICKED, NULL);
    }
    if (ui_freshsettingsRemove) {
        lv_obj_add_event_cb(ui_freshsettingsRemove, freshsettings_remove, LV_EVENT_ALL, NULL);
    }

    // --- Fresh Faults ---
    if (ui_freshfaultsBackButton) {
        lv_obj_add_flag(ui_freshfaultsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshfaultsBackButton, freshfaults_to_fresh, LV_EVENT_CLICKED, NULL);
    }


    // --- Waste  ---    
    if (ui_wasteBackButton) {
        lv_obj_add_flag(ui_wasteBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wasteBackButton, waste_to_home, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wasteFaultButton) {
        lv_obj_add_flag(ui_wasteFaultButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wasteFaultButton, waste_to_wastefaults, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastefaultsClearButton) {
        lv_obj_add_flag(ui_wastefaultsClearButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wastefaultsClearButton, waste_clear_faults, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wasteSettingsButton) {       
        lv_obj_add_flag(ui_wasteSettingsButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wasteSettingsButton, waste_to_wastesettings, LV_EVENT_CLICKED, NULL);
    }       
    
    
    // --- Waste Settings ---
    if (ui_wastesettingsBackButton) {
        lv_obj_add_flag(ui_wastesettingsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wastesettingsBackButton, wastesettings_to_waste, LV_EVENT_CLICKED, NULL);
    }   
    if (ui_wastesettingsDrainLevelButton) {
        lv_obj_add_event_cb(ui_wastesettingsDrainLevelButton, wastesettings_show_drain_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsOverlayBackButton) {
        lv_obj_add_event_cb(ui_wastesettingsOverlayBackButton, wastesettings_hide_drain_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsOverlayDrainSlider) {
        lv_obj_add_event_cb(ui_wastesettingsOverlayDrainSlider, wastesettings_slider_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_wastesettingsDiagnostic) {
        lv_obj_add_event_cb(ui_wastesettingsDiagnostic, wastesettings_show_diag_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingDiagnosticOverlayBackButton) {
        lv_obj_add_event_cb(ui_wastesettingDiagnosticOverlayBackButton, wastesettings_hide_diag_overlay, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsRestart) {
        lv_obj_add_event_cb(ui_wastesettingsRestart, wastesettings_restart, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsFreezeProtection) {
        lv_obj_add_event_cb(ui_wastesettingsFreezeProtection, wastesettings_freeze_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_wastesettingsSafetyOveride) {
        lv_obj_add_event_cb(ui_wastesettingsSafetyOveride, wastesettings_safety_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_wastesettingsValveOveride) {
        lv_obj_add_event_cb(ui_wastesettingsValveOveride, wastesettings_valve_changed, LV_EVENT_VALUE_CHANGED, NULL);
    }
    if (ui_wastesettingsSetFullButton) {
        lv_obj_add_event_cb(ui_wastesettingsSetFullButton, wastesettings_set_full, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsSetEmptyButton) {
        lv_obj_add_event_cb(ui_wastesettingsSetEmptyButton, wastesettings_set_empty, LV_EVENT_CLICKED, NULL);
    }
    if (ui_wastesettingsRemove) {
        lv_obj_add_event_cb(ui_wastesettingsRemove, wastesettings_remove, LV_EVENT_ALL, NULL);
    }


    // --- Waste Faults ---
    if (ui_wastefaultsBackButton) {
        lv_obj_add_flag(ui_wastefaultsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wastefaultsBackButton, wastefaults_to_waste, LV_EVENT_CLICKED, NULL);
    }   
    



}
