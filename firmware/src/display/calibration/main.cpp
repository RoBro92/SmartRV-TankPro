#include <Arduino.h>
#include <LovyanGFX.hpp>

#ifndef CYD_PANEL_ST7789
#define CYD_PANEL_ST7789 0
#endif

#ifndef CYD_TFT_SPI_HOST
#define CYD_TFT_SPI_HOST SPI2_HOST
#define CYD_TFT_SCLK 12
#define CYD_TFT_MOSI 11
#define CYD_TFT_MISO 13
#define CYD_TFT_DC 46
#define CYD_TFT_CS 10
#define CYD_TFT_RST -1
#define CYD_TFT_BL 45
#define CYD_TFT_RGB_ORDER 0
#endif

#ifndef CYD_TFT_INVERT
#define CYD_TFT_INVERT 0
#endif

#ifndef CYD_TOUCH_FT5X06
#define CYD_TOUCH_FT5X06 1
#endif
#ifndef CYD_TOUCH_SDA
#define CYD_TOUCH_SDA 16
#endif
#ifndef CYD_TOUCH_SCL
#define CYD_TOUCH_SCL 15
#endif
#ifndef CYD_TOUCH_IRQ
#define CYD_TOUCH_IRQ 17
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

class LGFX_CYD : public lgfx::LGFX_Device {
#if CYD_PANEL_ST7789
    lgfx::Panel_ST7789 _panel;
#else
    lgfx::Panel_ILI9341 _panel;
#endif
    lgfx::Bus_SPI _bus;
    lgfx::Touch_FT5x06 _touch;
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
            cfg.bus_shared = false;
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

        setPanel(&_panel);

        {
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
            cfg.offset_rotation = 2;  // rotate touch 180 so corners map correctly (BR->BR)
            _touch.config(cfg);
        }
        _panel.setTouch(&_touch);
    }
};

static LGFX_CYD lcd;

void draw_cross(uint16_t x, uint16_t y, uint16_t color) {
    const int16_t len = 6;
    lcd.drawLine(x - len, y, x + len, y, color);
    lcd.drawLine(x, y - len, x, y + len, color);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("CYD Touch Calibration Helper");

    lcd.init();
    lcd.setRotation(2);  // Portrait to match project
    lcd.setBrightness(255);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(TC_DATUM);
    lcd.drawString("Tap anywhere - logs X/Y", SCREEN_WIDTH / 2, 10);
    lcd.drawString("Use coords to tune X/Y min/max", SCREEN_WIDTH / 2, 26);
}

void loop() {
    uint16_t x = 0, y = 0;
    if (lcd.getTouch(&x, &y)) {
        // Clear previous marker area
        lcd.fillRect(0, 40, SCREEN_WIDTH, 30, TFT_BLACK);
        lcd.setTextDatum(TL_DATUM);
        lcd.setCursor(4, 44);
        lcd.printf("Touch: x=%u y=%u", x, y);
        lcd.setCursor(4, 60);
        lcd.printf("Raw pins OK");
        // Draw marker where pressed (persists so multiple taps show history)
        draw_cross(x, y, TFT_GREEN);
        lcd.drawCircle(x, y, 8, TFT_YELLOW);
        Serial.printf("TOUCH x=%u y=%u\n", x, y);
        delay(100);
    } else {
        delay(10);
    }
}
