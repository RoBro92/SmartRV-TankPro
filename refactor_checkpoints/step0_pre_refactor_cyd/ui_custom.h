#pragma once

#include <Arduino.h>
#include "cyd_state.h"

// Custom UI bindings that should survive SquareLine regenerations.
// Call ui_register_custom_actions() after ui_init().

void ui_register_custom_actions();
void pairing_ui_on_pair_ack(const String &controller_id, tank_role_t role);
void pairing_ui_on_unpair_complete(tank_role_t role);
void cyd_ui_set_theme_selector(int idx);
