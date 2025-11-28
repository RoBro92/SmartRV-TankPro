#include <Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <esp_timer.h>

extern "C" {
#include "ui.h"
}

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 320;
constexpr uint32_t LVGL_TICK_MS = 5;
constexpr uint16_t DRAW_BUF_LINES = 16;  // lines per buffer; keeps RAM use reasonable

class LGFX_CYD : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Touch_XPT2046 _touch;
    lgfx::Light_PWM _light;

public:
    LGFX_CYD() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = HSPI_HOST;  // CYD uses HSPI pins
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 14;   // CYD SCK
            cfg.pin_mosi = 13;   // CYD MOSI
            cfg.pin_miso = 12;   // CYD MISO
            cfg.pin_dc = 2;      // CYD D/C
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        {
            auto cfg = _panel.config();
            cfg.pin_cs = 15;     // CYD TFT_CS
            cfg.pin_rst = 4;     // CYD TFT_RST
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
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;  // Touch uses its own SPI lines
            _panel.config(cfg);
        }

        {
            auto cfg = _light.config();
            cfg.pin_bl = 21;     // CYD backlight
            cfg.invert = false;
            cfg.freq = 2000;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }

        // Bind panel before applying touch calibration
        setPanel(&_panel);

        {
            auto cfg = _touch.config();
            cfg.spi_host = VSPI_HOST;   // dedicated bus for touch
            cfg.freq = 2000000;
            // XPT2046 wiring from ESPHome calibration
            cfg.pin_sclk = 25;
            cfg.pin_mosi = 32;
            cfg.pin_miso = 39;
            cfg.pin_cs = 33;    // touch CS
            cfg.pin_int = 36;   // touch IRQ
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            // Mirror X by swapping raw bounds (right touches map to higher raw values)
            cfg.x_min = 3736;
            cfg.x_max = 201;
            cfg.y_min = 226;
            cfg.y_max = 3646;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
    }
};

static LGFX_CYD lcd;
static lv_display_t *display = nullptr;
static lv_indev_t *touch_indev = nullptr;
static esp_timer_handle_t lvgl_tick_timer = nullptr;
static uint32_t inactivity_timeout_ms = 0;
static uint32_t last_activity_ms = 0;
static bool display_sleep = false;
static uint8_t current_brightness_duty = 255;

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

static void lvgl_brightness_cb(lv_event_t *e) {
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    apply_brightness_from_slider(slider);
}

static void lvgl_timeout_cb(lv_event_t *e) {
    lv_obj_t *dd = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int sel = lv_dropdown_get_selected(dd);
    switch (sel) {
        case 0: inactivity_timeout_ms = 0; break;           // Never
        case 1: inactivity_timeout_ms = 30000; break;       // 30s
        case 2: inactivity_timeout_ms = 60000; break;       // 1m
        case 3: inactivity_timeout_ms = 120000; break;      // 2m
        default: inactivity_timeout_ms = 0; break;
    }
    Serial.printf("[timeout] selection=%d -> %lu ms\n", sel, static_cast<unsigned long>(inactivity_timeout_ms));
    last_activity_ms = millis();
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

void setup() {
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

    ui_init();

    // Attach brightness slider with 10–100% range
    if (ui_cydBrightnessSlider) {
        lv_slider_set_range(ui_cydBrightnessSlider, 0, 100);
        lv_slider_set_value(ui_cydBrightnessSlider, 100, LV_ANIM_OFF);
        apply_brightness_from_slider(ui_cydBrightnessSlider);
        lv_obj_add_event_cb(ui_cydBrightnessSlider, lvgl_brightness_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    }

    // Attach display timeout dropdown
    if (ui_cydTimeout) {
        lv_obj_add_event_cb(ui_cydTimeout, lvgl_timeout_cb, LV_EVENT_VALUE_CHANGED, nullptr);
        // Initialize timeout from current selection
        lv_obj_send_event(ui_cydTimeout, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

void loop() {
    lv_timer_handler();
    handle_inactivity();
    delay(5);
}
