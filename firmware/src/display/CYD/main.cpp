#include <Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <esp_timer.h>
#include <Preferences.h>
#include <esp_heap_caps.h>

extern "C" {
#include "ui.h"
}
#include "ui_custom.h"

#ifndef CYD_PANEL_ST7789
#define CYD_PANEL_ST7789 0
#endif

#ifndef CYD_TFT_SPI_HOST
// Default (ESP32) CYD pinout; override via build flags for new boards (e.g., S3 variant).
#define CYD_TFT_SPI_HOST HSPI_HOST
#define CYD_TFT_SCLK 14
#define CYD_TFT_MOSI 13
#define CYD_TFT_MISO 12
#define CYD_TFT_DC 2
#define CYD_TFT_CS 15
#define CYD_TFT_RST 4
#define CYD_TFT_BL 21
#define CYD_TFT_RGB_ORDER 0
#endif

#ifndef CYD_TFT_INVERT
#define CYD_TFT_INVERT 0
#endif

#ifndef CYD_TOUCH_SPI_HOST
#define CYD_TOUCH_SPI_HOST VSPI_HOST
#endif
#ifndef CYD_TOUCH_SCLK
#define CYD_TOUCH_SCLK 25
#endif
#ifndef CYD_TOUCH_MOSI
#define CYD_TOUCH_MOSI 32
#endif
#ifndef CYD_TOUCH_MISO
#define CYD_TOUCH_MISO 39
#endif
#ifndef CYD_TOUCH_CS
#define CYD_TOUCH_CS 33
#endif
#ifndef CYD_TOUCH_IRQ
#define CYD_TOUCH_IRQ 36
#endif
#ifndef CYD_TOUCH_SHARED
#define CYD_TOUCH_SHARED 0
#endif

#ifndef CYD_TOUCH_X_MIN
#define CYD_TOUCH_X_MIN 3736
#endif
#ifndef CYD_TOUCH_X_MAX
#define CYD_TOUCH_X_MAX 201
#endif
#ifndef CYD_TOUCH_Y_MIN
#define CYD_TOUCH_Y_MIN 226
#endif
#ifndef CYD_TOUCH_Y_MAX
#define CYD_TOUCH_Y_MAX 3646
#endif

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 320;
constexpr uint32_t LVGL_TICK_MS = 5;
constexpr uint16_t DRAW_BUF_LINES = 16;  // lines per buffer; keeps RAM use reasonable
constexpr uint8_t SETTINGS_VERSION = 1;

struct CydSettings {
    uint8_t version = SETTINGS_VERSION;
    uint8_t brightness_pct = 100;  // 0–100 from UI
    uint8_t timeout_index = 0;     // 0: Never, 1:30s, 2:1m, 3:2m
    uint8_t theme_index = 0;       // 0: Light, 1: Dark
};

class LGFX_CYD : public lgfx::LGFX_Device {
#if CYD_PANEL_ST7789
    lgfx::Panel_ST7789 _panel;
#else
    lgfx::Panel_ILI9341 _panel;
#endif
    lgfx::Bus_SPI _bus;
#if defined(CYD_TOUCH_FT5X06) && CYD_TOUCH_FT5X06
    lgfx::Touch_FT5x06 _touch;
#else
    lgfx::Touch_XPT2046 _touch;
#endif
    lgfx::Light_PWM _light;

public:
    LGFX_CYD() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = CYD_TFT_SPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = CYD_TFT_SCLK;
            cfg.pin_mosi = CYD_TFT_MOSI;
            cfg.pin_miso = CYD_TFT_MISO;
            cfg.pin_dc = CYD_TFT_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        {
            auto cfg = _panel.config();
            cfg.pin_cs = CYD_TFT_CS;
            cfg.pin_rst = CYD_TFT_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = SCREEN_WIDTH;
            cfg.panel_height = SCREEN_HEIGHT;
            cfg.memory_width = SCREEN_WIDTH;
            cfg.memory_height = SCREEN_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = CYD_TFT_INVERT;
            cfg.rgb_order = CYD_TFT_RGB_ORDER;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;  // Touch uses its own SPI lines
            _panel.config(cfg);
        }

        {
            auto cfg = _light.config();
            cfg.pin_bl = CYD_TFT_BL;
            cfg.invert = false;
            cfg.freq = 2000;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }

        // Bind panel before applying touch calibration
        setPanel(&_panel);

        {
#if defined(CYD_TOUCH_FT5X06) && CYD_TOUCH_FT5X06
            auto cfg = _touch.config();
            cfg.i2c_port = 0;
            cfg.freq = 400000;
            cfg.pin_sda = CYD_TOUCH_SDA;
            cfg.pin_scl = CYD_TOUCH_SCL;
            cfg.pin_int = CYD_TOUCH_IRQ;
            cfg.i2c_addr = 0x38;
            cfg.bus_shared = false;
            _touch.config(cfg);
#else
            auto cfg = _touch.config();
            cfg.spi_host = CYD_TOUCH_SPI_HOST;
            cfg.freq = 2000000;
            cfg.pin_sclk = CYD_TOUCH_SCLK;
            cfg.pin_mosi = CYD_TOUCH_MOSI;
            cfg.pin_miso = CYD_TOUCH_MISO;
            cfg.pin_cs = CYD_TOUCH_CS;
            cfg.pin_int = CYD_TOUCH_IRQ;
            cfg.bus_shared = CYD_TOUCH_SHARED;
            cfg.offset_rotation = 0;
            // Mirror X by swapping raw bounds (right touches map to higher raw values)
            cfg.x_min = CYD_TOUCH_X_MIN;
            cfg.x_max = CYD_TOUCH_X_MAX;
            cfg.y_min = CYD_TOUCH_Y_MIN;
            cfg.y_max = CYD_TOUCH_Y_MAX;
            _touch.config(cfg);
#endif
            _panel.setTouch(&_touch);
        }
    }
};

static LGFX_CYD lcd;
static lv_display_t *display = nullptr;
static lv_indev_t *touch_indev = nullptr;
static esp_timer_handle_t lvgl_tick_timer = nullptr;
static Preferences prefs;
static CydSettings settings;
static uint32_t inactivity_timeout_ms = 0;
static uint32_t last_activity_ms = 0;
static uint32_t last_heap_log_ms = 0;
static bool display_sleep = false;
static uint8_t current_brightness_duty = 255;

static void log_heap_stats(const char *tag) {
    const size_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    Serial.printf("[heap]%s free=%u largest=%u psram=%u\n", tag ? tag : "", static_cast<unsigned>(free_8bit),
                  static_cast<unsigned>(largest_8bit), static_cast<unsigned>(free_psram));
}

static void apply_brightness_from_slider(lv_obj_t *slider) {
    int val = lv_slider_get_value(slider);
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    // Map 0–100% to 10–100% actual duty to avoid complete off
    const float clamped_pct = 10.0f + (static_cast<float>(val) * 0.90f);
    const uint8_t duty = static_cast<uint8_t>((clamped_pct * 255.0f) / 100.0f);
    current_brightness_duty = duty;
    lcd.setBrightness(duty);
    Serial.printf("[brightness] ui=%d%% -> %0.1f%% (duty=%u)\n", val, clamped_pct, duty);
    last_activity_ms = millis();
}

static void lv_tick_cb(void * /*arg*/) {
    lv_tick_inc(LVGL_TICK_MS);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = lv_area_get_width(area);
    const uint32_t h = lv_area_get_height(area);

    // LovyanGFX will convert ARGB8888 (LVGL 32-bit color) to the panel's format.
    lcd.pushImage(area->x1, area->y1, w, h, reinterpret_cast<lgfx::argb8888_t *>(px_map));

    lv_display_flush_ready(disp);
}

static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    const bool touched = lcd.getTouch(&touch_x, &touch_y);

    if (display_sleep && touched) {
        // Wake on first touch, do not forward to widgets
        lcd.setBrightness(current_brightness_duty);
        display_sleep = false;
        last_activity_ms = millis();
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_x;
        data->point.y = touch_y;
        Serial.printf("[touch] x=%u y=%u\n", touch_x, touch_y);
        last_activity_ms = millis();
    }
}

static void apply_timeout_selection(int sel) {
    switch (sel) {
        case 0: inactivity_timeout_ms = 0; break;
        case 1: inactivity_timeout_ms = 30000; break;
        case 2: inactivity_timeout_ms = 60000; break;
        case 3: inactivity_timeout_ms = 120000; break;
        default: inactivity_timeout_ms = 0; break;
    }
    Serial.printf("[timeout] selection=%d -> %lu ms\n", sel, static_cast<unsigned long>(inactivity_timeout_ms));
}

static void handle_inactivity() {
    if (display_sleep || inactivity_timeout_ms == 0) return;
    const uint32_t now = millis();
    if (now - last_activity_ms >= inactivity_timeout_ms) {
        lcd.setBrightness(0);
        display_sleep = true;
        Serial.println("[timeout] display sleep");
    }
}

static void apply_theme_selection(int sel) {
    const bool dark = (sel == 1);
    lv_theme_t *theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), dark, LV_FONT_DEFAULT);
    lv_disp_set_theme(display, theme);
    Serial.printf("[theme] selection=%d (%s)\n", sel, dark ? "dark" : "light");
}

static void save_settings() {
    prefs.putBytes("cfg", &settings, sizeof(settings));
}

static void load_settings() {
    const size_t len = prefs.getBytesLength("cfg");
    if (len == sizeof(CydSettings)) {
        prefs.getBytes("cfg", &settings, sizeof(settings));
        const bool version_ok = settings.version == SETTINGS_VERSION;
        const bool ranges_ok = settings.brightness_pct <= 100 && settings.timeout_index <= 3 && settings.theme_index <= 1;
        if (!version_ok || !ranges_ok) {
            settings = CydSettings();  // reset to defaults
        }
    } else {
        settings = CydSettings();  // defaults when missing
    }
}

static void lvgl_theme_cb(lv_event_t *e) {
    lv_obj_t *dd = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int sel = lv_dropdown_get_selected(dd);
    if (settings.theme_index != sel) {
        settings.theme_index = static_cast<uint8_t>(sel);
        save_settings();
    }
    apply_theme_selection(sel);
    last_activity_ms = millis();
}

static void lvgl_brightness_cb(lv_event_t *e) {
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int val = lv_slider_get_value(slider);
    if (settings.brightness_pct != val) {
        settings.brightness_pct = static_cast<uint8_t>(val);
        save_settings();
    }
    apply_brightness_from_slider(slider);
}

static void lvgl_timeout_cb(lv_event_t *e) {
    lv_obj_t *dd = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int sel = lv_dropdown_get_selected(dd);
    if (settings.timeout_index != sel) {
        settings.timeout_index = static_cast<uint8_t>(sel);
        save_settings();
    }
    apply_timeout_selection(sel);
    last_activity_ms = millis();
}

void setup() {
    Serial.begin(115200);
    delay(200);  // give USB CDC a moment to connect
    Serial.println("[boot] CYD display starting");

    lcd.init();
    lcd.setRotation(2);  // Portrait: 240 x 320
    lcd.setBrightness(255);
    current_brightness_duty = 255;
    last_activity_ms = millis();

    lv_init();

    static lv_color_t draw_buf1[SCREEN_WIDTH * DRAW_BUF_LINES];

    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_flush_cb(display, lvgl_flush_cb);
    lv_display_set_buffers(display, draw_buf1, nullptr, sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(display);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);


    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, lvgl_touch_cb);
    lv_indev_set_display(touch_indev, display);

    esp_timer_create_args_t periodic_timer_args = {};
    periodic_timer_args.callback = lv_tick_cb;
    periodic_timer_args.name = "lv_tick";
    periodic_timer_args.dispatch_method = ESP_TIMER_TASK;
    periodic_timer_args.skip_unhandled_events = false;

    esp_timer_create(&periodic_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_MS * 1000);

    prefs.begin("cyd", false);
    load_settings();

    ui_init();
    ui_register_custom_actions();
    log_heap_stats(" setup");

    // Attach brightness slider with 10–100% range
    if (ui_cydBrightnessSlider) {
        lv_slider_set_range(ui_cydBrightnessSlider, 0, 100);
        lv_slider_set_value(ui_cydBrightnessSlider, settings.brightness_pct, LV_ANIM_OFF);
        apply_brightness_from_slider(ui_cydBrightnessSlider);
        lv_obj_add_event_cb(ui_cydBrightnessSlider, lvgl_brightness_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    }

    // Attach display timeout dropdown
    if (ui_cydTimeout) {
        lv_dropdown_set_selected(ui_cydTimeout, settings.timeout_index);
        lv_obj_add_event_cb(ui_cydTimeout, lvgl_timeout_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        apply_timeout_selection(settings.timeout_index);
    }

    // Attach theme dropdown
    if (ui_cydTheme) {
        lv_dropdown_set_selected(ui_cydTheme, settings.theme_index);
        lv_obj_add_event_cb(ui_cydTheme, lvgl_theme_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        apply_theme_selection(settings.theme_index);
    }
}

void loop() {
    lv_timer_handler();
    handle_inactivity();
    const uint32_t now = millis();
    if (now - last_heap_log_ms >= 5000) {
        log_heap_stats("");
        last_heap_log_ms = now;
    }
    delay(5);
}
