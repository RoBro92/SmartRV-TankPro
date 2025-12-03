#include <lvgl.h>
#include "ui.h"

// Keep navigation/overlay logic here so regenerating SquareLine files won't wipe it.

static void nav_to_settings(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_cydSettings, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_cydSettings_screen_init);
}

static void nav_to_fresh(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_fresh, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_fresh_screen_init);
}

static void nav_to_grey(lv_event_t *e) {
    LV_UNUSED(e);
    _ui_screen_change(&ui_Grey, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 0, &ui_Grey_screen_init);
}

static void show_boot_wifi(lv_event_t *e) {
    LV_UNUSED(e);
    if (ui_overlayBootWifi == NULL) return;
    lv_obj_clear_flag(ui_overlayBootWifi, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_overlayBootWifi, 255);
    lv_obj_move_foreground(ui_overlayBootWifi);
}

static void hide_boot_wifi(lv_event_t *e) {
    LV_UNUSED(e);
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

void ui_register_custom_actions() {
    // Home navigation
    lv_obj_t *settings_btn = ui_comp_get_child(ui_titlepanel1, UI_COMP_TITLEPANEL_IMGBUTTON1);
    if (settings_btn) {
        lv_obj_add_event_cb(settings_btn, nav_to_settings, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeFreshCard) {
        lv_obj_add_flag(ui_homeFreshCard, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_homeFreshCard, nav_to_fresh, LV_EVENT_CLICKED, NULL);
    }
    if (ui_homeGreyCard) {
        lv_obj_add_flag(ui_homeGreyCard, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_homeGreyCard, nav_to_grey, LV_EVENT_CLICKED, NULL);
    }

    // Boot screen overlays
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
}
