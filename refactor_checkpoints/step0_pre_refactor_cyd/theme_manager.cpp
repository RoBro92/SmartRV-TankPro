#include "theme_manager.h"

#include <Arduino.h>
#include "cyd_state.h"
#include "ui.h"

namespace {

Preferences *prefs = nullptr;
lv_display_t *display_ref = nullptr;
theme_t current_theme = THEME_LIGHT;
constexpr const char *kThemeKey = "theme_idx";

lv_color_t light_bg = lv_color_hex(0xffffff);
lv_color_t light_text = lv_color_hex(0x000000);
lv_color_t dark_bg = lv_color_hex(0x222222);
lv_color_t dark_text = lv_color_hex(0xffffff);
lv_color_t root_bg = lv_color_hex(0x000000);
lv_color_t control_light_bg = lv_color_hex(0xCCC5C5);
lv_color_t control_dark_bg = lv_color_hex(0x8D8787);
lv_color_t fault_red = lv_color_hex(0xFF0000);

bool is_excluded(lv_obj_t *obj) {
    return obj == ui_cydsettingsDiagnosticOverlay || obj == ui_freshsettingsDiagnosticOverlay ||
           obj == ui_wastesettingsDiagnosticOverlay || obj == ui_cydsettingsFactoryResetConfirmationOverlay ||
           obj == ui_freshFaultPopupOverlay2 || obj == ui_greyFaultPopupOverlay2 ||
           obj == ui_overlayBootWifi ||
           obj == ui_freshfaults || obj == ui_wastefaults;
}

void apply_text_to_descendants(lv_obj_t *obj, lv_color_t text) {
    if (!obj) return;
    if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_set_style_text_color(obj, text, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    const uint32_t child_count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_count; ++i) {
        apply_text_to_descendants(lv_obj_get_child(obj, i), text);
    }
}

// Apply theme to a specific panel and its textual children only.
void apply_theme_to_panel(lv_obj_t *panel, lv_color_t bg, lv_color_t text) {
    if (!panel) return;
    if (is_excluded(panel)) return;
    // Only style if this is a plain panel/container.
    if (!lv_obj_check_type(panel, &lv_obj_class)) return;

    lv_obj_set_style_bg_color(panel, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(panel, bg, LV_PART_MAIN | LV_STATE_DEFAULT);

    apply_text_to_descendants(panel, text);
}

// Apply theme text colour to status labels.
void apply_status_label(lv_obj_t *label, lv_color_t text) {
    if (!label) return;
    if (is_excluded(label)) return;
    // If currently red (fault/wet), leave it; state logic will clear it when fault clears.
    lv_color32_t curr32 = lv_color_to_32(lv_obj_get_style_text_color(label, LV_PART_MAIN | LV_STATE_DEFAULT),
                                         LV_OPA_COVER);
    if (lv_color32_eq(curr32, lv_color_to_32(fault_red, LV_OPA_COVER))) return;
    lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void apply_control_theme(lv_obj_t *obj, lv_color_t bg, lv_color_t text) {
    if (!obj) return;
    if (is_excluded(obj)) return;
    lv_obj_set_style_bg_color(obj, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(obj, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, text, LV_PART_MAIN | LV_STATE_DEFAULT);
    apply_text_to_descendants(obj, text);

    // Dropdown list part should also match the control styling.
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_obj_t *list = lv_dropdown_get_list(obj);
        if (list) {
            lv_obj_set_style_bg_color(list, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(list, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(list, bg, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(list, text, LV_PART_MAIN | LV_STATE_DEFAULT);
            apply_text_to_descendants(list, text);
        }
    }
}

void apply_current_theme() {
    lv_color_t bg = (current_theme == THEME_DARK) ? dark_bg : light_bg;
    lv_color_t text = (current_theme == THEME_DARK) ? dark_text : light_text;
    // Keep root backgrounds black.
    lv_obj_t *roots[] = {ui_boot, ui_home, ui_fresh, ui_waste, ui_cydsettings, ui_freshsettings, ui_wastesettings,
                         ui_freshfaults, ui_wastefaults, nullptr};
    for (lv_obj_t **ptr = roots; *ptr != nullptr; ++ptr) {
        if (*ptr) {
            lv_obj_set_style_bg_color(*ptr, root_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // Explicit list of themable panels only.
    lv_obj_t *panels[] = {ui_freshsettingsMainCard, ui_freshMainCard, ui_freshControlPanel, ui_wastemaincard,
                          ui_wastecontrolpanel, ui_homeFreshCard, ui_homeWasteCard, ui_cydsettingspanel, nullptr};
    for (lv_obj_t **pp = panels; *pp != nullptr; ++pp) {
        apply_theme_to_panel(*pp, bg, text);
    }

    // Status labels on home should respect theme unless fault red is active.
    apply_status_label(ui_homeFreshStatusLabel, text);
    apply_status_label(ui_homeGreyStatusLabel, text);
    apply_status_label(ui_freshLeakLabel, text);
    apply_status_label(ui_wasteLeakLabel, text);

    // Update leak base colors to match the current theme so future clears use the right text color.
    cyd_state_update_leak_base_colors(text);

    // Theme dropdown, timeout, and units controls.
    lv_color_t control_bg = (current_theme == THEME_DARK) ? control_dark_bg : control_light_bg;
    lv_color_t control_text = (current_theme == THEME_DARK) ? dark_text : light_text;
    apply_control_theme(ui_cydTheme, control_bg, control_text);
    apply_control_theme(ui_cydTimeout, control_bg, control_text);
    apply_control_theme(ui_cydUnits, control_bg, control_text);
    apply_control_theme(ui_freshsettingsFreezeProtection, control_bg, control_text);
    apply_control_theme(ui_wastesettingsFreezeProtection, control_bg, control_text);
}

}  // namespace

void theme_manager_init(Preferences *prefs_handle, lv_display_t *disp) {
    prefs = prefs_handle;
    display_ref = disp;
    uint8_t stored = 0;
    if (prefs) {
        stored = prefs->getUChar(kThemeKey, 0);
    }
    if (stored > 1) stored = 0;
    current_theme = static_cast<theme_t>(stored);
    Serial.printf("[theme] loaded %u\n", stored);
}

theme_t theme_manager_get(void) {
    return current_theme;
}

void theme_manager_set(theme_t theme) {
    if (theme != THEME_LIGHT && theme != THEME_DARK) return;
    if (theme == current_theme) {
        apply_current_theme();
        return;
    }
    current_theme = theme;
    if (prefs) {
        prefs->putUChar(kThemeKey, static_cast<uint8_t>(theme));
    }
    Serial.printf("[theme] set to %s\n", theme == THEME_DARK ? "dark" : "light");
    apply_current_theme();
}

void theme_manager_apply_current(void) {
    apply_current_theme();
}
