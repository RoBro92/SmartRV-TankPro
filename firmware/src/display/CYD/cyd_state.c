#include "cyd_state.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <lvgl.h>
#include "ui.h"

cyd_state_t cyd_state;

static const uint8_t LEVEL_INVALID = CYD_LEVEL_INVALID;
static const uint8_t SETTING_INVALID = CYD_SETTING_INVALID;
static const uint16_t VOLT_INVALID = CYD_VOLT_INVALID;
static const uint16_t FAULT_INVALID = CYD_FAULT_INVALID;
static lv_color_t home_fresh_status_base;
static bool home_fresh_status_base_set = false;
static lv_color_t home_waste_status_base;
static bool home_waste_status_base_set = false;
static lv_color_t fresh_leak_base;
static bool fresh_leak_base_set = false;
static lv_color_t waste_leak_base;
static bool waste_leak_base_set = false;
static lv_color_t fresh_fill_base;
static bool fresh_fill_base_set = false;
static lv_color_t fresh_drain_base;
static bool fresh_drain_base_set = false;
static lv_color_t waste_drain_base;
static bool waste_drain_base_set = false;

void cyd_state_init_defaults(void) {
    memset(&cyd_state, 0, sizeof(cyd_state));

    // User-facing labels and defaults; tweak here if you rebrand screens.
    strncpy(cyd_state.fresh.name, "Fresh Tank", sizeof(cyd_state.fresh.name) - 1);
    cyd_state.fresh.role = TANK_ROLE_FRESH;
    cyd_state.fresh.level_percent = LEVEL_INVALID;
    cyd_state.fresh.paired = false;
    cyd_state.fresh.temp_c = NAN;
    cyd_state.fresh.status = TANK_STATUS_OK;
    cyd_state.fresh.leak = false;
    cyd_state.fresh.freeze_enabled = false;
    cyd_state.fresh.freeze_setting = SETTING_INVALID;
    cyd_state.fresh.fault_code = FAULT_INVALID;
    cyd_state.fresh.fault_description[0] = '\0';
    cyd_state.fresh.stop_level_percent = SETTING_INVALID;
    cyd_state.fresh.full_voltage_mv = VOLT_INVALID;
    cyd_state.fresh.empty_voltage_mv = VOLT_INVALID;
    cyd_state.fresh.safety_override_enabled = false;
    cyd_state.fresh.valve_override_enabled = false;
    cyd_state.fresh.restart_requested = false;
    cyd_state.fresh.diag_ip[0] = '\0';
    cyd_state.fresh.diag_id[0] = '\0';
    cyd_state.fresh.diag_mac[0] = '\0';
    cyd_state.fresh.diag_status[0] = '\0';
    cyd_state.fresh.diag_role[0] = '\0';
    cyd_state.fresh.diag_uptime_s = 0;
    cyd_state.fresh.diag_signal_dbm = 0;
    cyd_state.fresh.diag_version[0] = '\0';

    strncpy(cyd_state.waste.name, "Waste Tank", sizeof(cyd_state.waste.name) - 1);
    cyd_state.waste.role = TANK_ROLE_WASTE;
    cyd_state.waste.level_percent = LEVEL_INVALID;
    cyd_state.waste.paired = false;
    cyd_state.waste.temp_c = NAN;
    cyd_state.waste.status = TANK_STATUS_OK;
    cyd_state.waste.leak = false;
    cyd_state.waste.freeze_enabled = false;
    cyd_state.waste.freeze_setting = SETTING_INVALID;
    cyd_state.waste.fault_code = FAULT_INVALID;
    cyd_state.waste.fault_description[0] = '\0';
    cyd_state.waste.stop_level_percent = SETTING_INVALID;
    cyd_state.waste.full_voltage_mv = VOLT_INVALID;
    cyd_state.waste.empty_voltage_mv = VOLT_INVALID;
    cyd_state.waste.safety_override_enabled = false;
    cyd_state.waste.valve_override_enabled = false;
    cyd_state.waste.restart_requested = false;
    cyd_state.waste.diag_ip[0] = '\0';
    cyd_state.waste.diag_id[0] = '\0';
    cyd_state.waste.diag_mac[0] = '\0';
    cyd_state.waste.diag_status[0] = '\0';
    cyd_state.waste.diag_role[0] = '\0';
    cyd_state.waste.diag_uptime_s = 0;
    cyd_state.waste.diag_signal_dbm = 0;
    cyd_state.waste.diag_version[0] = '\0';

    strncpy(cyd_state.firmware_version, "V 0.2.0", sizeof(cyd_state.firmware_version) - 1);
    cyd_state.setup_complete = false;
    cyd_state.wifi_connected = false;
    cyd_state.wifi_ssid[0] = '\0';
    cyd_state.wifi_ip[0] = '\0';
    cyd_state.diag_id[0] = '\0';
    cyd_state.diag_ip[0] = '\0';
    cyd_state.diag_mac[0] = '\0';
    cyd_state.diag_status[0] = '\0';
    cyd_state.diag_signal_dbm = INT16_MIN;
    cyd_state.diag_uptime_s = 0;
}

static const char *cyd_tank_status_to_string(tank_status_t status) {
    switch (status) {
        case TANK_STATUS_FILL: return "Fill";
        case TANK_STATUS_DRAIN: return "Drain";
        case TANK_STATUS_FAULT: return "Fault";
        case TANK_STATUS_OK:
        default: return "OK";
    }
}

static bool s_units_metric = true;

void cyd_state_update_leak_base_colors(lv_color_t text_color) {
    fresh_leak_base = text_color;
    fresh_leak_base_set = true;
    waste_leak_base = text_color;
    waste_leak_base_set = true;
    home_fresh_status_base = text_color;
    home_fresh_status_base_set = true;
    home_waste_status_base = text_color;
    home_waste_status_base_set = true;
}

void cyd_state_set_units_metric(bool metric) {
    s_units_metric = metric;
}

bool cyd_state_units_metric(void) {
    return s_units_metric;
}

void cyd_state_apply_to_home_screen(void) {
    const bool fresh_assigned = cyd_state.fresh.diag_id[0] != '\0';
    const bool waste_assigned = cyd_state.waste.diag_id[0] != '\0';
    const bool fresh_valid = fresh_assigned && cyd_state.fresh.level_percent != LEVEL_INVALID;
    const bool waste_valid = waste_assigned && cyd_state.waste.level_percent != LEVEL_INVALID;
    const bool wifi_ok = cyd_state.wifi_connected;
    if (ui_homeFreshSetupButton) {
        if (fresh_assigned) {
            lv_obj_add_flag(ui_homeFreshSetupButton, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(ui_homeFreshSetupButton, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ui_homeWasteSetupButton) {
        if (waste_assigned) {
            lv_obj_add_flag(ui_homeWasteSetupButton, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(ui_homeWasteSetupButton, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ui_homeheaderWifiConnected) {
        if (wifi_ok) {
            lv_obj_clear_flag(ui_homeheaderWifiConnected, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_homeheaderWifiConnected, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ui_homeFreshLevelArc) {
        lv_arc_set_value(ui_homeFreshLevelArc, fresh_valid ? cyd_state.fresh.level_percent : 0);
    }
    if (ui_homeFreshLevelLabel) {
        if (fresh_valid) lv_label_set_text_fmt(ui_homeFreshLevelLabel, "%u%%", cyd_state.fresh.level_percent);
        else lv_label_set_text(ui_homeFreshLevelLabel, "--");
    }
    if (ui_homeFreshTempLabel) {
        if (fresh_assigned && !isnan(cyd_state.fresh.temp_c)) {
            char buf[16];
            float temp = s_units_metric ? cyd_state.fresh.temp_c : (cyd_state.fresh.temp_c * 9.0f / 5.0f) + 32.0f;
            int rounded = (int) lroundf(temp);
            snprintf(buf, sizeof(buf), "%d°%c", rounded, s_units_metric ? 'C' : 'F');
            lv_label_set_text(ui_homeFreshTempLabel, buf);
        } else {
            lv_label_set_text(ui_homeFreshTempLabel, "--");
        }
    }
    if (ui_homeFreshStatusLabel) {
        if (!home_fresh_status_base_set) {
            home_fresh_status_base = lv_obj_get_style_text_color(ui_homeFreshStatusLabel, LV_PART_MAIN);
            home_fresh_status_base_set = true;
        }
        if (!fresh_assigned) {
            lv_label_set_text(ui_homeFreshStatusLabel, "--");
            lv_obj_set_style_text_color(ui_homeFreshStatusLabel, home_fresh_status_base,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_label_set_text(ui_homeFreshStatusLabel, cyd_tank_status_to_string(cyd_state.fresh.status));
            if (cyd_state.fresh.status == TANK_STATUS_FAULT) {
                lv_obj_set_style_text_color(ui_homeFreshStatusLabel, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_text_color(ui_homeFreshStatusLabel, home_fresh_status_base,
                                            LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }

    if (ui_homeGreyLevelArc) {
        lv_arc_set_value(ui_homeGreyLevelArc, waste_valid ? cyd_state.waste.level_percent : 0);
    }
    if (ui_homeGreyLevelLabel) {
        if (waste_valid) lv_label_set_text_fmt(ui_homeGreyLevelLabel, "%u%%", cyd_state.waste.level_percent);
        else lv_label_set_text(ui_homeGreyLevelLabel, "--");
    }
    if (ui_homeGreyTempLabel) {
        if (cyd_state.waste.paired && !isnan(cyd_state.waste.temp_c)) {
            char buf[16];
            float temp = s_units_metric ? cyd_state.waste.temp_c : (cyd_state.waste.temp_c * 9.0f / 5.0f) + 32.0f;
            int rounded = (int) lroundf(temp);
            snprintf(buf, sizeof(buf), "%d°%c", rounded, s_units_metric ? 'C' : 'F');
            lv_label_set_text(ui_homeGreyTempLabel, buf);
        } else {
            lv_label_set_text(ui_homeGreyTempLabel, "--");
        }
    }
    if (ui_homeGreyStatusLabel) {
        if (!home_waste_status_base_set) {
            home_waste_status_base = lv_obj_get_style_text_color(ui_homeGreyStatusLabel, LV_PART_MAIN);
            home_waste_status_base_set = true;
        }
        if (!waste_assigned) {
            lv_label_set_text(ui_homeGreyStatusLabel, "--");
            lv_obj_set_style_text_color(ui_homeGreyStatusLabel, home_waste_status_base,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_label_set_text(ui_homeGreyStatusLabel, cyd_tank_status_to_string(cyd_state.waste.status));
            if (cyd_state.waste.status == TANK_STATUS_FAULT) {
                lv_obj_set_style_text_color(ui_homeGreyStatusLabel, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_text_color(ui_homeGreyStatusLabel, home_waste_status_base,
                                            LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }
}

void cyd_state_apply_to_fresh_screen(void) {
    const bool valid = cyd_state.fresh.paired && cyd_state.fresh.level_percent != LEVEL_INVALID;
    if (ui_freshLevelBar) {
        lv_bar_set_value(ui_freshLevelBar, valid ? cyd_state.fresh.level_percent : 0, LV_ANIM_OFF);
    }
    if (ui_freshLevelLabel) {
        if (valid) lv_label_set_text_fmt(ui_freshLevelLabel, "%u%%", cyd_state.fresh.level_percent);
        else lv_label_set_text(ui_freshLevelLabel, "--");
    }
    if (ui_freshTempLabel) {
        if (cyd_state.fresh.paired && !isnan(cyd_state.fresh.temp_c)) {
            char buf[16];
            float temp = s_units_metric ? cyd_state.fresh.temp_c : (cyd_state.fresh.temp_c * 9.0f / 5.0f) + 32.0f;
            int rounded = (int) lroundf(temp);
            snprintf(buf, sizeof(buf), "%d°%c", rounded, s_units_metric ? 'C' : 'F');
            lv_label_set_text(ui_freshTempLabel, buf);
        } else {
            lv_label_set_text(ui_freshTempLabel, "--");
        }
    }
    if (ui_FreshStatusLabel) {
        if (!cyd_state.fresh.paired) lv_label_set_text(ui_FreshStatusLabel, "--");
        else lv_label_set_text(ui_FreshStatusLabel, cyd_tank_status_to_string(cyd_state.fresh.status));
    }
    if (ui_freshLeakLabel) {
        if (!fresh_leak_base_set) {
            fresh_leak_base = lv_obj_get_style_text_color(ui_freshLeakLabel, LV_PART_MAIN);
            fresh_leak_base_set = true;
        }
        if (!cyd_state.fresh.paired) {
            lv_label_set_text(ui_freshLeakLabel, "--");
            lv_obj_set_style_text_color(ui_freshLeakLabel, fresh_leak_base, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            const bool wet = cyd_state.fresh.leak;
            lv_label_set_text(ui_freshLeakLabel, wet ? "Wet" : "Dry");
            lv_obj_set_style_text_color(ui_freshLeakLabel,
                                        wet ? lv_palette_main(LV_PALETTE_RED) : fresh_leak_base,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    if (ui_freshFreezeLabel) {
        if (!cyd_state.fresh.paired) lv_label_set_text(ui_freshFreezeLabel, "--");
        else lv_label_set_text(ui_freshFreezeLabel,
                               (cyd_state.fresh.freeze_setting > 0) ? "On" : "Off");
    }
    if (ui_freshFaultButtonLabel) {
        if (!cyd_state.fresh.paired || cyd_state.fresh.fault_code == FAULT_INVALID) {
            lv_label_set_text(ui_freshFaultButtonLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_freshFaultButtonLabel, "%u", cyd_state.fresh.fault_code);
        }
    }
    if (ui_freshFillButton) {
        if (!fresh_fill_base_set) {
            fresh_fill_base = lv_obj_get_style_bg_color(ui_freshFillButton, LV_PART_MAIN);
            fresh_fill_base_set = true;
        }
        const bool active = cyd_state.fresh.status == TANK_STATUS_FILL;
        lv_obj_set_style_bg_color(ui_freshFillButton,
                                  active ? lv_palette_main(LV_PALETTE_GREEN) : fresh_fill_base,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_freshDrainButton) {
        if (!fresh_drain_base_set) {
            fresh_drain_base = lv_obj_get_style_bg_color(ui_freshDrainButton, LV_PART_MAIN);
            fresh_drain_base_set = true;
        }
        const bool active = cyd_state.fresh.status == TANK_STATUS_DRAIN;
        lv_obj_set_style_bg_color(ui_freshDrainButton,
                                  active ? lv_palette_main(LV_PALETTE_GREEN) : fresh_drain_base,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void cyd_state_apply_to_waste_screen(void) {
    const bool valid = cyd_state.waste.paired && cyd_state.waste.level_percent != LEVEL_INVALID;
    if (ui_wasteLevelBar) {
        lv_bar_set_value(ui_wasteLevelBar, valid ? cyd_state.waste.level_percent : 0, LV_ANIM_OFF);
    }
    if (ui_wasteLevelLabel) {
        if (valid) lv_label_set_text_fmt(ui_wasteLevelLabel, "%u%%", cyd_state.waste.level_percent);
        else lv_label_set_text(ui_wasteLevelLabel, "--");
    }
    if (ui_wasteTempLabel) {
        if (cyd_state.waste.paired && !isnan(cyd_state.waste.temp_c)) {
            char buf[16];
            float temp = s_units_metric ? cyd_state.waste.temp_c : (cyd_state.waste.temp_c * 9.0f / 5.0f) + 32.0f;
            int rounded = (int) lroundf(temp);
            snprintf(buf, sizeof(buf), "%d°%c", rounded, s_units_metric ? 'C' : 'F');
            lv_label_set_text(ui_wasteTempLabel, buf);
        } else {
            lv_label_set_text(ui_wasteTempLabel, "--");
        }
    }
    if (ui_wasteStatusLabel) {
        if (!cyd_state.waste.paired) lv_label_set_text(ui_wasteStatusLabel, "--");
        else lv_label_set_text(ui_wasteStatusLabel, cyd_tank_status_to_string(cyd_state.waste.status));
    }
    if (ui_wasteLeakLabel) {
        if (!waste_leak_base_set) {
            waste_leak_base = lv_obj_get_style_text_color(ui_wasteLeakLabel, LV_PART_MAIN);
            waste_leak_base_set = true;
        }
        if (!cyd_state.waste.paired) {
            lv_label_set_text(ui_wasteLeakLabel, "--");
            lv_obj_set_style_text_color(ui_wasteLeakLabel, waste_leak_base, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            const bool wet = cyd_state.waste.leak;
            lv_label_set_text(ui_wasteLeakLabel, wet ? "Wet" : "Dry");
            lv_obj_set_style_text_color(ui_wasteLeakLabel,
                                        wet ? lv_palette_main(LV_PALETTE_RED) : waste_leak_base,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    if (ui_wasteFreezeLabel) {
        if (!cyd_state.waste.paired) lv_label_set_text(ui_wasteFreezeLabel, "--");
        else lv_label_set_text(ui_wasteFreezeLabel,
                               (cyd_state.waste.freeze_setting > 0) ? "On" : "Off");
    }
    if (ui_wasteFaultButtonLabel) {
        if (!cyd_state.waste.paired || cyd_state.waste.fault_code == FAULT_INVALID) {
            lv_label_set_text(ui_wasteFaultButtonLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_wasteFaultButtonLabel, "%u", cyd_state.waste.fault_code);
        }
    }
    if (ui_WasteDrainButton) {
        if (!waste_drain_base_set) {
            waste_drain_base = lv_obj_get_style_bg_color(ui_WasteDrainButton, LV_PART_MAIN);
            waste_drain_base_set = true;
        }
        const bool active = cyd_state.waste.status == TANK_STATUS_DRAIN;
        lv_obj_set_style_bg_color(ui_WasteDrainButton,
                                  active ? lv_palette_main(LV_PALETTE_GREEN) : waste_drain_base,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void cyd_state_apply_to_freshfaults_screen(void) {
    if (ui_freshfaultsCodeLabel) {
        if (!cyd_state.fresh.paired || cyd_state.fresh.fault_code == FAULT_INVALID) {
            lv_label_set_text(ui_freshfaultsCodeLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_freshfaultsCodeLabel, "%u", cyd_state.fresh.fault_code);
        }
    }
    if (ui_freshfaultsCodeDescription) {
        if (!cyd_state.fresh.paired) {
            lv_label_set_text(ui_freshfaultsCodeDescription, "No Controller Connected");
        } else if (cyd_state.fresh.fault_code == FAULT_INVALID || cyd_state.fresh.status != TANK_STATUS_FAULT) {
            lv_label_set_text(ui_freshfaultsCodeDescription, "No Active Fault");
        } else {
            lv_label_set_text(ui_freshfaultsCodeDescription, cyd_state.fresh.fault_description);
        }
    }
}

void cyd_state_apply_to_wastefaults_screen(void) {
    if (ui_wastefaultsCodeLabel) {
        if (!cyd_state.waste.paired || cyd_state.waste.fault_code == FAULT_INVALID) {
            lv_label_set_text(ui_wastefaultsCodeLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_wastefaultsCodeLabel, "%u", cyd_state.waste.fault_code);
        }
    }
    if (ui_wastefaultsCodeDescription) {
        if (!cyd_state.waste.paired) {
            lv_label_set_text(ui_wastefaultsCodeDescription, "No Controller Connected");
        } else if (cyd_state.waste.fault_code == FAULT_INVALID || cyd_state.waste.status != TANK_STATUS_FAULT) {
            lv_label_set_text(ui_wastefaultsCodeDescription, "No Active Fault");
        } else {
            lv_label_set_text(ui_wastefaultsCodeDescription, cyd_state.waste.fault_description);
        }
    }
}

static void apply_fill_stop(lv_obj_t *slider, lv_obj_t *label, uint8_t percent) {
    if (slider) {
        lv_slider_set_value(slider, percent == SETTING_INVALID ? 0 : percent, LV_ANIM_OFF);
    }
    if (label) {
        if (percent == SETTING_INVALID) lv_label_set_text(label, "--");
        else lv_label_set_text_fmt(label, "%u%%", percent);
    }
}

static void apply_voltage(lv_obj_t *label, uint16_t mv) {
    if (!label) return;
    if (mv == VOLT_INVALID) lv_label_set_text(label, "--");
    else lv_label_set_text_fmt(label, "%.2fV", mv / 1000.0f);
}

static void apply_toggle(lv_obj_t *sw, bool on) {
    if (!sw) return;
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw, LV_STATE_CHECKED);
}

static void apply_freeze_options(lv_obj_t *dd, uint8_t setting) {
    if (!dd) return;
    char opts[64];
    if (s_units_metric) {
        snprintf(opts, sizeof(opts), "Off\n1\n2\n3\n4\n5");
    } else {
        int f1 = lroundf(1.0f * 9.0f / 5.0f + 32.0f);
        int f2 = lroundf(2.0f * 9.0f / 5.0f + 32.0f);
        int f3 = lroundf(3.0f * 9.0f / 5.0f + 32.0f);
        int f4 = lroundf(4.0f * 9.0f / 5.0f + 32.0f);
        int f5 = lroundf(5.0f * 9.0f / 5.0f + 32.0f);
        snprintf(opts, sizeof(opts), "Off\n%d\n%d\n%d\n%d\n%d", f1, f2, f3, f4, f5);
    }
    lv_dropdown_set_options(dd, opts);
    uint8_t sel = setting;
    if (sel > 5) sel = 0;
    lv_dropdown_set_selected(dd, sel);
}

void cyd_state_apply_to_freshsettings_screen(void) {
    apply_fill_stop(ui_freshsettingsOverlayFillSlider, ui_freshsettingsOverlayFillPercentage, cyd_state.fresh.stop_level_percent);
    if (ui_freshsettingsFillStopLevelLabel) {
        if (cyd_state.fresh.stop_level_percent == SETTING_INVALID || !cyd_state.fresh.paired) {
            lv_label_set_text(ui_freshsettingsFillStopLevelLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_freshsettingsFillStopLevelLabel, "%u%%", cyd_state.fresh.stop_level_percent);
        }
    }
    apply_voltage(ui_freshsettingsFullVoltage, cyd_state.fresh.full_voltage_mv);
    apply_voltage(ui_freshsettingsEmptyVoltage, cyd_state.fresh.empty_voltage_mv);
    apply_freeze_options(ui_freshsettingsFreezeProtection, cyd_state.fresh.freeze_setting);
    apply_toggle(ui_freshsettingsSafetyOveride, cyd_state.fresh.safety_override_enabled);
    apply_toggle(ui_freshsettingsValveOveride, cyd_state.fresh.valve_override_enabled);
}

void cyd_state_apply_to_wastesettings_screen(void) {
    apply_fill_stop(ui_wastesettingsOverlayDrainSlider, ui_wastesettingsOverlayDrainPercentage, cyd_state.waste.stop_level_percent);
    if (ui_wastesettingsDrainStopLevelLabel) {
        if (cyd_state.waste.stop_level_percent == SETTING_INVALID || !cyd_state.waste.paired) {
            lv_label_set_text(ui_wastesettingsDrainStopLevelLabel, "--");
        } else {
            lv_label_set_text_fmt(ui_wastesettingsDrainStopLevelLabel, "%u%%", cyd_state.waste.stop_level_percent);
        }
    }
    apply_voltage(ui_wastesettingsFullVoltage, cyd_state.waste.full_voltage_mv);
    apply_voltage(ui_wastesettingsEmptyVoltage, cyd_state.waste.empty_voltage_mv);
    apply_freeze_options(ui_wastesettingsFreezeProtection, cyd_state.waste.freeze_setting);
    apply_toggle(ui_wastesettingsSafetyOveride, cyd_state.waste.safety_override_enabled);
    apply_toggle(ui_wastesettingsValveOveride, cyd_state.waste.valve_override_enabled);
}

void cyd_state_apply_to_cydsettings_screen(void) {
    if (ui_cydFirmwareLabel) {
        lv_label_set_text(ui_cydFirmwareLabel, cyd_state.firmware_version);
    }
}

void cyd_state_apply_to_cydsettings_diag_overlay(void) {
    const bool wifi_ok = cyd_state.wifi_connected;
    if (ui_cydsettingsdiagoverlayIP) {
        lv_label_set_text(ui_cydsettingsdiagoverlayIP,
                          (wifi_ok && cyd_state.wifi_ip[0]) ? cyd_state.wifi_ip : "Not connected");
    }
    if (ui_cydsettingsdiagoverlayID) {
        lv_label_set_text(ui_cydsettingsdiagoverlayID,
                          cyd_state.diag_id[0] ? cyd_state.diag_id : "--");
    }
    if (ui_cydsettingsdiagoverlayMAC) {
        lv_label_set_text(ui_cydsettingsdiagoverlayMAC,
                          cyd_state.diag_mac[0] ? cyd_state.diag_mac : "--");
    }
    if (ui_cydsettingsdiagoverlayWifiStatus) {
        lv_label_set_text(ui_cydsettingsdiagoverlayWifiStatus, wifi_ok ? "Connected" : "Not connected");
    }
    if (ui_cydsettingsdiagoverlayWifiSignal) {
        if (wifi_ok && cyd_state.diag_signal_dbm != INT16_MIN) {
            lv_label_set_text_fmt(ui_cydsettingsdiagoverlayWifiSignal, "%d dBm", cyd_state.diag_signal_dbm);
        } else {
            lv_label_set_text(ui_cydsettingsdiagoverlayWifiSignal, "--");
        }
    }
    if (ui_cydsettingsdiagoverlayUptime) {
        const uint32_t secs = cyd_state.diag_uptime_s;
        if (wifi_ok && secs > 0) {
            const uint32_t mins = secs / 60;
            const uint32_t rem = secs % 60;
            lv_label_set_text_fmt(ui_cydsettingsdiagoverlayUptime, "%lum %02lus",
                                  (unsigned long)mins, (unsigned long)rem);
        } else {
            lv_label_set_text(ui_cydsettingsdiagoverlayUptime, "--");
        }
    }
    if (ui_cydsettingsdiagoverlayFreshConnected) {
        lv_label_set_text(ui_cydsettingsdiagoverlayFreshConnected, cyd_state.fresh.paired ? "Yes" : "No");
    }
    if (ui_cydsettingsdiagoverlayWasteConnected) {
        lv_label_set_text(ui_cydsettingsdiagoverlayWasteConnected, cyd_state.waste.paired ? "Yes" : "No");
    }
}

void cyd_state_apply_to_boot_screen(void) {
    if (ui_bootFirmwareLabel) {
        lv_label_set_text(ui_bootFirmwareLabel, cyd_state.firmware_version);
    }
}

static void apply_diag_overlay(const tank_state_t *t, lv_obj_t *ip, lv_obj_t *id, lv_obj_t *mac,
                               lv_obj_t *status, lv_obj_t *role, lv_obj_t *uptime,
                               lv_obj_t *signal, lv_obj_t *version) {
    if (!t->paired) {
        if (ip) lv_label_set_text(ip, "Not connected");
        if (id) lv_label_set_text(id, "--");
        if (mac) lv_label_set_text(mac, "--");
        if (status) lv_label_set_text(status, "Not connected");
        if (role) lv_label_set_text(role, "--");
        if (uptime) lv_label_set_text(uptime, "--");
        if (signal) lv_label_set_text(signal, "--");
        if (version) lv_label_set_text(version, "--");
        return;
    }
    if (ip) lv_label_set_text(ip, t->diag_ip[0] ? t->diag_ip : "--");
    if (id) lv_label_set_text(id, t->diag_id[0] ? t->diag_id : "--");
    if (mac) lv_label_set_text(mac, t->diag_mac[0] ? t->diag_mac : "--");
    if (status) lv_label_set_text(status, t->diag_status[0] ? t->diag_status : "--");
    if (role) lv_label_set_text(role, t->diag_role[0] ? t->diag_role : "--");
    if (uptime) {
        if (t->diag_uptime_s == 0) {
            lv_label_set_text(uptime, "--");
        } else {
            uint32_t mins = t->diag_uptime_s / 60;
            uint32_t secs = t->diag_uptime_s % 60;
            lv_label_set_text_fmt(uptime, "%lum %02lus", (unsigned long)mins, (unsigned long)secs);
        }
    }
    if (signal) {
        if (t->diag_signal_dbm == 0) lv_label_set_text(signal, "--");
        else lv_label_set_text_fmt(signal, "%d dBm", t->diag_signal_dbm);
    }
    if (version) lv_label_set_text(version, t->diag_version[0] ? t->diag_version : "--");
}

void cyd_state_apply_to_freshsettings_diag_overlay(void) {
    apply_diag_overlay(&cyd_state.fresh,
                       ui_freshsettingsdiagoverlayIP,
                       ui_freshsettingsdiagoverlayID,
                       ui_freshsettingsdiagoverlayMAC,
                       ui_freshsettingsdiagoverlayStatus,
                       ui_freshsettingsdiagoverlayRole,
                       ui_freshsettingsdiagoverlayUptime,
                       ui_freshsettingsdiagoverlaySignal,
                       ui_freshsettingsdiagoverlayVersion);
}

void cyd_state_apply_to_wastesettings_diag_overlay(void) {
    apply_diag_overlay(&cyd_state.waste,
                       ui_wastesettingsdiagoverlayIP,
                       ui_wastesettingsdiagoverlayID,
                       ui_wastesettingsdiagoverlayMAC,
                       ui_wastesettingsdiagoverlayStatus,
                       ui_wastesettingsdiagoverlayRole,
                       ui_wastesettingsdiagoverlayUptime,
                       ui_wastesettingsdiagoverlaySignal,
                       ui_wastesettingsdiagoverlayVersion);
}
