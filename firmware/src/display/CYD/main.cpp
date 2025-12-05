#include <Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <esp_timer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_heap_caps.h>

extern "C" {
#include "ui.h"
}
#include "ui_custom.h"
#include "cyd_state.h"

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
#define CYD_TFT_RGB_ORDER 0  // Use RGB order to match LVGL buffer when using RGB565
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
constexpr const char *SETUP_FLAG_KEY = "setup_done";
constexpr const char *WIFI_SSID_KEY = "wifi_ssid";
constexpr const char *WIFI_PASS_KEY = "wifi_pass";

struct CydSettings {
    uint8_t version = SETTINGS_VERSION;
    uint8_t brightness_pct = 100;  // 0–100 from UI
    uint8_t timeout_index = 0;     // 0: Never, 1:30s, 2:1m, 3:2m
    uint8_t theme_index = 0;       // 0: Light, 1: Dark
    uint8_t units_index = 0;       // 0: Metric (C), 1: Imperial (F)
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
            cfg.x_min = CYD_TOUCH_X_MIN;
            cfg.x_max = CYD_TOUCH_X_MAX;
            cfg.y_min = CYD_TOUCH_Y_MIN;
            cfg.y_max = CYD_TOUCH_Y_MAX;
            cfg.offset_rotation = 0;  // no additional rotation; matches current panel setup
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
static bool setup_complete = false;

struct OnboardingContext {
    bool active = false;
    String ap_ssid;
    String ap_pass;
    WebServer server{80};
    DNSServer dns;
} onboarding;

void start_wifi_onboarding();
void stop_wifi_onboarding();
void handle_onboarding();
static void mark_setup_complete_and_persist();
static void mark_setup_complete_direct();

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

    lcd.pushImage(area->x1, area->y1, w, h, reinterpret_cast<lgfx::rgb565_t *>(px_map));

    lv_display_flush_ready(disp);
}

static void lvgl_touch_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data) {
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    const bool touched = lcd.getTouch(&touch_x, &touch_y);
    constexpr int16_t TOUCH_X_OFFSET_PX = 0;  // no manual offset

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
        int32_t corrected_x = static_cast<int32_t>(touch_x) - TOUCH_X_OFFSET_PX;
        if (corrected_x < 0) corrected_x = 0;
        if (corrected_x >= SCREEN_WIDTH) corrected_x = SCREEN_WIDTH - 1;
        touch_x = static_cast<uint16_t>(corrected_x);
        if (touch_y >= SCREEN_HEIGHT) touch_y = SCREEN_HEIGHT - 1;
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
        // Only force-return to Home after initial setup is complete; otherwise just sleep the display.
        if (setup_complete) {
            _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
        }
        Serial.println("[timeout] display sleep");
    }
}

static void apply_theme_selection(int sel) {
    const bool dark = (sel == 1);
    lv_theme_t *theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), dark, LV_FONT_DEFAULT);
    lv_display_set_theme(display, theme);
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
        const bool ranges_ok = settings.brightness_pct <= 100 && settings.timeout_index <= 3 && settings.theme_index <= 1 &&
                               settings.units_index <= 1;
        if (!version_ok || !ranges_ok) {
            settings = CydSettings();  // reset to defaults
        }
    } else {
        settings = CydSettings();  // defaults when missing
    }
}

static void load_setup_flag() {
    setup_complete = prefs.getBool(SETUP_FLAG_KEY, false);
    cyd_state.setup_complete = setup_complete;
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

static void lvgl_units_cb(lv_event_t *e) {
    lv_obj_t *dd = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int sel = lv_dropdown_get_selected(dd);
    if (settings.units_index != sel) {
        settings.units_index = static_cast<uint8_t>(sel);
        save_settings();
    }
    cyd_state_set_units_metric(settings.units_index == 0);
    cyd_state_apply_to_home_screen();
    cyd_state_apply_to_fresh_screen();
    cyd_state_apply_to_waste_screen();
    cyd_state_apply_to_freshsettings_screen();
    cyd_state_apply_to_wastesettings_screen();
    last_activity_ms = millis();
}

static String make_mac_suffix() {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
    return String(buf);
}

static String make_temp_password() {
    static const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    String pass;
    pass.reserve(8);
    for (int i = 0; i < 8; i++) {
        uint32_t r = esp_random();
        pass += charset[r % (sizeof(charset) - 1)];
    }
    return pass;
}

static void update_boot_wifi_labels() {
    if (ui_bootWifissid) {
        lv_label_set_text(ui_bootWifissid, onboarding.ap_ssid.c_str());
    }
    if (ui_bootWifipass) {
        lv_label_set_text(ui_bootWifipass, onboarding.ap_pass.c_str());
    }
}

void stop_wifi_onboarding() {
    if (!onboarding.active) return;
    onboarding.dns.stop();
    onboarding.server.stop();
    WiFi.softAPdisconnect(true);
    onboarding.active = false;
}

static void handle_scan_route() {
    // Faster active scan; keep hidden networks out to reduce noise
    int16_t n = WiFi.scanNetworks(false /*async*/, false /*show_hidden*/, false /*passive*/, 200 /*ms/chan*/);
    String page = R"HTML(
<!DOCTYPE html><html><head>
<title>TankPro CYD Scan</title>
<style>
body { font-family: Arial, sans-serif; background:#f4f6f8; color:#0a0a0a; margin:0; padding:0; }
.wrap { max-width: 520px; margin: 0 auto; padding: 28px; }
.card { background:#ffffff; padding:20px; border-radius:10px; box-shadow:0 4px 10px rgba(0,0,0,0.08); }
h1 { font-size: 22px; margin:0 0 12px 0; }
.field { margin: 12px 0; }
.field label { display:block; font-weight:600; margin-bottom:6px; }
select, input { width:100%; padding:10px; font-size:16px; border:1px solid #c7ccd1; border-radius:6px; }
button { width:100%; padding:12px; font-size:16px; border:none; border-radius:6px; background:#2563eb; color:white; font-weight:700; cursor:pointer; }
button:hover { background:#1d4ed8; }
a { color:#2563eb; font-weight:600; }
</style>
</head><body><div class="wrap"><div class="card">
<h1>Select a network</h1>
<form action="/connect" method="POST">
  <div class="field"><label>Networks</label>
    <select name="ssid">
)HTML";
    if (n <= 0) {
        page += "<option value=\"\" selected disabled>No networks found. Go back and retry.</option>";
    } else {
        for (int i = 0; i < n; i++) {
            page += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
        }
        page += "<option value=\"\">Other (enter manually below)</option>";
    }
    page += R"HTML(    </select>
  </div>
  <div class="field"><label>Or enter SSID</label><input name="ssid_other" placeholder="Your Wi‑Fi name"></div>
  <div class="field"><label>Password</label><input name="pass" type="password" placeholder="Wi‑Fi password"></div>
  <button type="submit">Connect</button>
</form>
<p style="text-align:center; margin-top:14px;"><a href="/">Back</a></p>
</div></div></body></html>)HTML";
    onboarding.server.send(200, "text/html", page);
}

static void handle_root_route() {
    String page = R"HTML(
<!DOCTYPE html><html><head>
<title>TankPro CYD Setup</title>
<style>
body { font-family: Arial, sans-serif; background:#f4f6f8; color:#0a0a0a; margin:0; padding:0; }
.wrap { max-width: 520px; margin: 0 auto; padding: 28px; }
.card { background:#ffffff; padding:20px; border-radius:10px; box-shadow:0 4px 10px rgba(0,0,0,0.08); }
h1 { font-size: 24px; margin:0 0 12px 0; }
p { margin: 8px 0; }
.list { padding-left: 18px; }
.field { margin: 12px 0; }
.field label { display:block; font-weight:600; margin-bottom:6px; }
.field input { width:100%; padding:10px; font-size:16px; border:1px solid #c7ccd1; border-radius:6px; }
button { width:100%; padding:12px; font-size:16px; border:none; border-radius:6px; background:#2563eb; color:white; font-weight:700; cursor:pointer; }
button:hover { background:#1d4ed8; }
a { color:#2563eb; font-weight:600; }
</style>
</head><body><div class="wrap"><div class="card">
<h1>TankPro CYD Wi‑Fi Setup</h1>
<p>Connect to this AP:</p>
<ul class="list">
<li><strong>SSID:</strong> )HTML";
    page += onboarding.ap_ssid;
    page += "</li><li><strong>Password:</strong> ";
    page += onboarding.ap_pass;
    page += R"HTML(</li></ul>
<p>Enter the Wi‑Fi network you want the CYD to join:</p>
<form action="/connect" method="POST">
  <div class="field"><label>Network SSID</label><input name="ssid" placeholder="Your Wi‑Fi name"></div>
  <div class="field"><label>Password</label><input name="pass" type="password" placeholder="Wi‑Fi password"></div>
  <button type="submit">Connect</button>
</form>
<p style="text-align:center; margin-top:14px;"><a href="/scan">Scan nearby networks</a></p>
</div></div></body></html>)HTML";
    onboarding.server.send(200, "text/html", page);
}

static void handle_connect_route() {
    String ssid = onboarding.server.arg("ssid");
    if (ssid.isEmpty()) {
        ssid = onboarding.server.arg("ssid_other");
    }
    const String pass = onboarding.server.arg("pass");
    if (ssid.isEmpty()) {
        onboarding.server.send(400, "text/html", "SSID required. <a href=\"/\">Back</a>");
        return;
    }
    onboarding.server.send(200, "text/html", "Connecting... device will reboot on success.");
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long start = millis();
    bool ok = false;
    while (millis() - start < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
            ok = true;
            break;
        }
        delay(200);
    }
    if (ok) {
        prefs.putString(WIFI_SSID_KEY, ssid);
        prefs.putString(WIFI_PASS_KEY, pass);
        mark_setup_complete_and_persist();
        stop_wifi_onboarding();
        delay(500);
        ESP.restart();
    } else {
        WiFi.disconnect();
    }
}

void start_wifi_onboarding() {
    onboarding.ap_ssid = "TankProCYD-" + make_mac_suffix();
    onboarding.ap_pass = make_temp_password();
    WiFi.mode(WIFI_AP_STA);
    IPAddress apIP(192, 168, 4, 1);
    IPAddress gw(192, 168, 4, 1);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gw, mask);
    WiFi.softAP(onboarding.ap_ssid.c_str(), onboarding.ap_pass.c_str());
    onboarding.dns.start(53, "*", apIP);
    // Captive-portal friendly probes and handlers
    onboarding.server.on("/", HTTP_GET, handle_root_route);
    onboarding.server.on("/scan", HTTP_GET, handle_scan_route);
    onboarding.server.on("/connect", HTTP_POST, handle_connect_route);
    onboarding.server.on("/generate_204", HTTP_GET, handle_root_route);        // Android/ChromeOS
    onboarding.server.on("/gen_204", HTTP_GET, handle_root_route);             // some variants
    onboarding.server.on("/hotspot-detect.html", HTTP_GET, handle_root_route); // iOS/macOS
    onboarding.server.on("/ncsi.txt", HTTP_GET, handle_root_route);            // Windows
    onboarding.server.on("/connecttest.txt", HTTP_GET, handle_root_route);     // Windows alt
    onboarding.server.on("/success.html", HTTP_GET, handle_root_route);        // Amazon/others
    onboarding.server.onNotFound(handle_root_route);
    onboarding.server.begin();
    onboarding.active = true;
    update_boot_wifi_labels();
}

void handle_onboarding() {
    if (!onboarding.active) return;
    onboarding.dns.processNextRequest();
    onboarding.server.handleClient();
}

static void mark_setup_complete_and_persist() {
    setup_complete = true;
    cyd_state.setup_complete = true;
    prefs.putBool(SETUP_FLAG_KEY, true);
}

static void mark_setup_complete_direct() {
    mark_setup_complete_and_persist();
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
    cyd_state_init_defaults();

    static lv_color_t draw_buf1[SCREEN_WIDTH * DRAW_BUF_LINES];
    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
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
    load_setup_flag();

    ui_init();
    ui_register_custom_actions();
    cyd_state_apply_to_home_screen();
    cyd_state_apply_to_boot_screen();
    if (setup_complete) {
        _ui_screen_change(&ui_home, LV_SCR_LOAD_ANIM_NONE, 0, 0, NULL);
        cyd_state_apply_to_home_screen();
    }
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

    // Attach units dropdown
    if (ui_cydUnits) {
        cyd_state_set_units_metric(settings.units_index == 0);
        lv_dropdown_set_selected(ui_cydUnits, settings.units_index);
        lv_obj_add_event_cb(ui_cydUnits, lvgl_units_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    } else {
        cyd_state_set_units_metric(true);
    }

    // Apply static info to settings
    cyd_state_apply_to_cydsettings_screen();
}

void loop() {
    lv_timer_handler();
    handle_onboarding();
    handle_inactivity();
    const uint32_t now = millis();
    if (now - last_heap_log_ms >= 5000) {
        log_heap_stats("");
        last_heap_log_ms = now;
    }
    delay(5);
}
