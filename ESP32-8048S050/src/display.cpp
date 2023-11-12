#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <display.h>
#include <driver/i2c.h>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lvgl.h>
#include <ui/ui.h>

// Display settings
#define screenWidth 800
#define screenHeight 480

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

class LGFX : public lgfx::LGFX_Device {

  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_GT911 _touch_instance;

  public:
  LGFX(void) {
    {
      auto cfg = _panel_instance.config();

      cfg.memory_width = 800;
      cfg.memory_height = 480;
      cfg.panel_width = 800;
      cfg.panel_height = 480;

      cfg.offset_x = 0;
      cfg.offset_y = 0;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _panel_instance.config_detail();

      cfg.use_psram = 1;

      _panel_instance.config_detail(cfg);
    }

    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0 = GPIO_NUM_8;   // B0
      cfg.pin_d1 = GPIO_NUM_3;   // B1
      cfg.pin_d2 = GPIO_NUM_46;  // B2
      cfg.pin_d3 = GPIO_NUM_9;   // B3
      cfg.pin_d4 = GPIO_NUM_1;   // B4
      cfg.pin_d5 = GPIO_NUM_5;   // G0
      cfg.pin_d6 = GPIO_NUM_6;   // G1
      cfg.pin_d7 = GPIO_NUM_7;   // G2
      cfg.pin_d8 = GPIO_NUM_15;  // G3
      cfg.pin_d9 = GPIO_NUM_16;  // G4
      cfg.pin_d10 = GPIO_NUM_4;  // G5
      cfg.pin_d11 = GPIO_NUM_45; // R0
      cfg.pin_d12 = GPIO_NUM_48; // R1
      cfg.pin_d13 = GPIO_NUM_47; // R2
      cfg.pin_d14 = GPIO_NUM_21; // R3
      cfg.pin_d15 = GPIO_NUM_14; // R4

      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync = GPIO_NUM_41;
      cfg.pin_hsync = GPIO_NUM_39;
      cfg.pin_pclk = GPIO_NUM_42;
      cfg.freq_write = 16000000;

      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch = 8;
      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch = 8;
      cfg.pclk_idle_high = 1;
      _bus_instance.config(cfg);
    }

    _panel_instance.setBus(&_bus_instance);

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = GPIO_NUM_2;
      _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    {
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;
      cfg.x_max = 800;
      cfg.y_min = 0;
      cfg.y_max = 480;
      cfg.pin_int = GPIO_NUM_NC;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      // I2C接続
      cfg.i2c_port = I2C_NUM_1;
      cfg.pin_sda = GPIO_NUM_19;
      cfg.pin_scl = GPIO_NUM_20;
      cfg.pin_rst = GPIO_NUM_38;
      cfg.freq = 400000;
      cfg.i2c_addr = 0x14; // 0x5D , 0x14
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance); // Sets the panel to use.
  }
};

LGFX tft;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);
  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

void ButClick(lv_event_t *e) {
  Serial0.println("Clicked");
}

void DisplayTasks() {
  lv_task_handler();
}

void InitDisplay() {
  tft.begin();
  lv_init();

  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

  /* Initialize the display */
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  /* Backlight */
  pinMode(GPIO_NUM_2, OUTPUT);
  digitalWrite(GPIO_NUM_2, HIGH);

  ui_init();
}