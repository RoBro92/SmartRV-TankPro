#include <lvgl.h>
#include <WiFi.h>
#include <cstdio>
#include "ui.h"
#include "cyd_state.h"

// Onboarding control (defined in main.cpp)
void start_wifi_onboarding();
void stop_wifi_onboarding();

// Keep navigation/overlay logic here so regenerating SquareLine files won't wipe it.
// To add a new navigation:
// 1) Write a small handler that calls _ui_screen_change(target_screen, animation, time_ms, delay_ms, target_init_fn).
// 2) Register it on the source widget(s) in ui_register_custom_actions().

// Navigation targets

// --- Boot ---
static void show_boot_wifi(lv_event_t *e) {
    LV_UNUSED(e);
    start_wifi_onboarding();
    if (ui_overlayBootWifi == NULL) return;
    lv_obj_clear_flag(ui_overlayBootWifi, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_overlayBootWifi, 255);
    lv_obj_move_foreground(ui_overlayBootWifi);
    // Disable the "Next" button until Wi-Fi is connected
    if (ui_buttonBootWifiNext) {
        if (WiFi.status() == WL_CONNECTED) {
            lv_obj_clear_state(ui_buttonBootWifiNext, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(ui_buttonBootWifiNext, LV_STATE_DISABLED);
        }
    }
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
    if (ui_overlayBootDirect == NULL) return;
    lv_obj_clear_flag(ui_overlayBootDirect, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_overlayBootDirect, 255);
    lv_obj_move_foreground(ui_overlayBootDirect);
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
}

static void freshsettings_show_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsDiagnosticOverlay) return;
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
}

static void freshsettings_safety_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsSafetyOveride) return;
    cyd_state.fresh.safety_override_enabled = lv_obj_has_state(ui_freshsettingsSafetyOveride, LV_STATE_CHECKED);
}

static void freshsettings_valve_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_freshsettingsValveOveride) return;
    cyd_state.fresh.valve_override_enabled = lv_obj_has_state(ui_freshsettingsValveOveride, LV_STATE_CHECKED);
}

static void freshsettings_set_full(lv_event_t *e) {
    LV_UNUSED(e);
    cyd_state.fresh.full_voltage_mv = static_cast<uint16_t>((cyd_state.fresh.full_voltage_mv + 50) % 5000);
    if (cyd_state.fresh.full_voltage_mv < 1000) cyd_state.fresh.full_voltage_mv = 3300;
    cyd_state_apply_to_freshsettings_screen();
}

static void freshsettings_set_empty(lv_event_t *e) {
    LV_UNUSED(e);
    cyd_state.fresh.empty_voltage_mv = static_cast<uint16_t>((cyd_state.fresh.empty_voltage_mv + 25) % 2000);
    if (cyd_state.fresh.empty_voltage_mv < 200) cyd_state.fresh.empty_voltage_mv = 800;
    cyd_state_apply_to_freshsettings_screen();
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
}

static void wastesettings_show_diag_overlay(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsDiagnosticOverlay) return;
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
}

static void wastesettings_safety_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsSafetyOveride) return;
    cyd_state.waste.safety_override_enabled = lv_obj_has_state(ui_wastesettingsSafetyOveride, LV_STATE_CHECKED);
}

static void wastesettings_valve_changed(lv_event_t *e) {
    LV_UNUSED(e);
    if (!ui_wastesettingsValveOveride) return;
    cyd_state.waste.valve_override_enabled = lv_obj_has_state(ui_wastesettingsValveOveride, LV_STATE_CHECKED);
}

static void wastesettings_set_full(lv_event_t *e) {
    LV_UNUSED(e);
    cyd_state.waste.full_voltage_mv = static_cast<uint16_t>((cyd_state.waste.full_voltage_mv + 50) % 5000);
    if (cyd_state.waste.full_voltage_mv < 1000) cyd_state.waste.full_voltage_mv = 3100;
    cyd_state_apply_to_wastesettings_screen();
}

static void wastesettings_set_empty(lv_event_t *e) {
    LV_UNUSED(e);
    cyd_state.waste.empty_voltage_mv = static_cast<uint16_t>((cyd_state.waste.empty_voltage_mv + 25) % 2000);
    if (cyd_state.waste.empty_voltage_mv < 200) cyd_state.waste.empty_voltage_mv = 700;
    cyd_state_apply_to_wastesettings_screen();
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
    if (ui_buttonBootWifiNext) {
        lv_obj_add_flag(ui_buttonBootWifiNext, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_buttonBootWifiNext, boot_to_home, LV_EVENT_CLICKED, NULL);
    }
    if (ui_buttonbootdirectscan) {
        lv_obj_add_flag(ui_buttonbootdirectscan, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_buttonbootdirectscan, boot_to_home, LV_EVENT_CLICKED, NULL);
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
    // Debug button: tap to cycle levels for visual testing
    if (ui_homeWasteButton) {
        lv_obj_add_event_cb(ui_homeWasteButton, home_debug_adjust, LV_EVENT_LONG_PRESSED, NULL);
    }

    // --- Cyd Settings ---
    if (ui_cydsettingsBackButton) {
        lv_obj_add_flag(ui_cydsettingsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_cydsettingsBackButton, cydsettings_to_home, LV_EVENT_CLICKED, NULL);
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
    if (ui_freshSettingsButton) {       
        lv_obj_add_flag(ui_freshSettingsButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_freshSettingsButton, fresh_to_freshsettings, LV_EVENT_CLICKED, NULL);
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


    // --- Waste Faults ---
    if (ui_wastefaultsBackButton) {
        lv_obj_add_flag(ui_wastefaultsBackButton, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_wastefaultsBackButton, wastefaults_to_waste, LV_EVENT_CLICKED, NULL);
    }   
    



}
