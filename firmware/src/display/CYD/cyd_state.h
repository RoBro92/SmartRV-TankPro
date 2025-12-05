#ifndef CYD_STATE_H
#define CYD_STATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TANK_ROLE_NONE = 0,
    TANK_ROLE_FRESH = 1,
    TANK_ROLE_WASTE = 2
} tank_role_t;

typedef enum {
    TANK_STATUS_OK = 0,
    TANK_STATUS_FILL,
    TANK_STATUS_DRAIN,
    TANK_STATUS_FAULT
} tank_status_t;

typedef struct {
    bool paired;
    char name[16];
    int8_t role;            // tank_role_t
    uint8_t level_percent;  // 0–100, 0xFF when unset
    float temp_c;           // NaN when unset
    tank_status_t status;
    bool leak;
    bool freeze_enabled;
    uint8_t freeze_setting;       // 0=Off, 1..5 from settings dropdown, 0xFF unset
    uint16_t fault_code;          // 0xFFFF when unset
    char fault_description[64];   // empty when unset
    uint8_t stop_level_percent;   // fill/drain stop threshold (%), 0xFF unset
    uint16_t full_voltage_mv;     // calibration, 0xFFFF unset
    uint16_t empty_voltage_mv;    // calibration, 0xFFFF unset
    bool safety_override_enabled;
    bool valve_override_enabled;
    bool restart_requested;
    char diag_ip[32];
    char diag_id[32];
    char diag_mac[24];
    char diag_status[24];
    char diag_role[16];
    uint32_t diag_uptime_s;
    int16_t diag_signal_dbm;
    char diag_version[16];
} tank_state_t;

typedef struct {
    tank_state_t fresh;
    tank_state_t waste;
    char firmware_version[16];
    bool setup_complete;
} cyd_state_t;

extern cyd_state_t cyd_state;

void cyd_state_init_defaults(void);
void cyd_state_apply_to_home_screen(void);
void cyd_state_apply_to_fresh_screen(void);
void cyd_state_apply_to_waste_screen(void);
void cyd_state_apply_to_freshsettings_screen(void);
void cyd_state_apply_to_wastesettings_screen(void);
void cyd_state_apply_to_cydsettings_screen(void);
void cyd_state_apply_to_freshsettings_diag_overlay(void);
void cyd_state_apply_to_wastesettings_diag_overlay(void);
void cyd_state_apply_to_boot_screen(void);
void cyd_state_set_units_metric(bool metric);
bool cyd_state_units_metric(void);
void cyd_state_apply_to_freshfaults_screen(void);
void cyd_state_apply_to_wastefaults_screen(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CYD_STATE_H
