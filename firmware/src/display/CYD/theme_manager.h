#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <lvgl.h>
#include <Preferences.h>

typedef enum {
    THEME_LIGHT = 0,
    THEME_DARK = 1
} theme_t;

void theme_manager_init(Preferences *prefs_handle, lv_display_t *disp);
theme_t theme_manager_get(void);
void theme_manager_set(theme_t theme);
void theme_manager_apply_current(void);

#endif  // THEME_MANAGER_H
