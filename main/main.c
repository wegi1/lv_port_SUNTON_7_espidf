/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lvgl_port.h"
#include "lv_demos.h"

#include "esp_lcd_touch_gt911.h"

#include "driver/ledc.h"

// Skonfigurujmy parametry PWM dla pinu GPIO 2
#define BL_LEDC_TIMER LEDC_TIMER_0
#define BL_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define BL_LEDC_DUTY_RES LEDC_TIMER_8_BIT // Zakres regulacji 0 - 255
#define BL_LEDC_FREQUENCY 5000            // 5 kHz (brak migotania)

/* LCD size */
#define EXAMPLE_LCD_H_RES (800)
#define EXAMPLE_LCD_V_RES (480)

/* LCD settings */
#define EXAMPLE_LCD_LVGL_FULL_REFRESH (0)
#define EXAMPLE_LCD_LVGL_DIRECT_MODE (1)
#define EXAMPLE_LCD_LVGL_AVOID_TEAR (1)
#define EXAMPLE_LCD_RGB_BOUNCE_BUFFER_MODE (1)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (0)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (100)
#define EXAMPLE_LCD_RGB_BUFFER_NUMS (2)
#define EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT (10)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_VSYNC (GPIO_NUM_40)
#define EXAMPLE_LCD_GPIO_HSYNC (GPIO_NUM_39)
#define EXAMPLE_LCD_GPIO_DE (GPIO_NUM_41)
#define EXAMPLE_LCD_GPIO_PCLK (GPIO_NUM_42)
#define EXAMPLE_LCD_GPIO_DISP (GPIO_NUM_NC)

// 🔵 BLUE CHANNEL (Sunton B0 - B4)
#define EXAMPLE_LCD_GPIO_DATA0 (GPIO_NUM_15) // B0 (lowest)
#define EXAMPLE_LCD_GPIO_DATA1 (GPIO_NUM_7)  // B1
#define EXAMPLE_LCD_GPIO_DATA2 (GPIO_NUM_6)  // B2
#define EXAMPLE_LCD_GPIO_DATA3 (GPIO_NUM_5)  // B3
#define EXAMPLE_LCD_GPIO_DATA4 (GPIO_NUM_4)  // B4 (highest)

// 🟢 GREEN CHANNEL (Sunton G0 - G5)
#define EXAMPLE_LCD_GPIO_DATA5 (GPIO_NUM_9)  // G0 (lowest)
#define EXAMPLE_LCD_GPIO_DATA6 (GPIO_NUM_46) // G1
#define EXAMPLE_LCD_GPIO_DATA7 (GPIO_NUM_3)  // G2
#define EXAMPLE_LCD_GPIO_DATA8 (GPIO_NUM_8)  // G3
#define EXAMPLE_LCD_GPIO_DATA9 (GPIO_NUM_16) // G4
#define EXAMPLE_LCD_GPIO_DATA10 (GPIO_NUM_1) // G5 (highest)

// 🔴 RED CHANNEL (Sunton R0 - R4)
#define EXAMPLE_LCD_GPIO_DATA11 (GPIO_NUM_14) // R0 (lowest)
#define EXAMPLE_LCD_GPIO_DATA12 (GPIO_NUM_21) // R1
#define EXAMPLE_LCD_GPIO_DATA13 (GPIO_NUM_47) // R2
#define EXAMPLE_LCD_GPIO_DATA14 (GPIO_NUM_48) // R3
#define EXAMPLE_LCD_GPIO_DATA15 (GPIO_NUM_45) // R4 (highest)

#define GPIO_LCD_BL (GPIO_NUM_2)

/* Touch settings */
#define EXAMPLE_TOUCH_I2C_NUM (0)
#define EXAMPLE_TOUCH_I2C_CLK_HZ (400000)

/* LCD touch pins */
#define EXAMPLE_TOUCH_I2C_SCL (GPIO_NUM_20)
#define EXAMPLE_TOUCH_I2C_SDA (GPIO_NUM_19)

#define EXAMPLE_LCD_PANEL_35HZ_RGB_TIMING() \
    {                                       \
        .pclk_hz = 20 * 1000 * 1000,        \
        .h_res = EXAMPLE_LCD_H_RES,         \
        .v_res = EXAMPLE_LCD_V_RES,         \
        .hsync_pulse_width = 1,             \
        .hsync_back_porch = 42,             \
        .hsync_front_porch = 20,            \
        .vsync_pulse_width = 10,            \
        .vsync_back_porch = 12,             \
        .vsync_front_porch = 4,             \
        .flags.pclk_active_neg = true,      \
    }

static const char *TAG = "EXAMPLE";

// LVGL image declare
LV_IMG_DECLARE(esp_logo)

/* LCD IO and panel */
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD initialization */
    ESP_LOGI(TAG, "Initialize RGB panel");
    esp_lcd_rgb_panel_config_t panel_conf = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = 64,
        .data_width = 16,
        .bits_per_pixel = 16,
        .de_gpio_num = EXAMPLE_LCD_GPIO_DE,
        .pclk_gpio_num = EXAMPLE_LCD_GPIO_PCLK,
        .vsync_gpio_num = EXAMPLE_LCD_GPIO_VSYNC,
        .hsync_gpio_num = EXAMPLE_LCD_GPIO_HSYNC,
        .disp_gpio_num = EXAMPLE_LCD_GPIO_DISP,
        .data_gpio_nums = {
            EXAMPLE_LCD_GPIO_DATA0,
            EXAMPLE_LCD_GPIO_DATA1,
            EXAMPLE_LCD_GPIO_DATA2,
            EXAMPLE_LCD_GPIO_DATA3,
            EXAMPLE_LCD_GPIO_DATA4,
            EXAMPLE_LCD_GPIO_DATA5,
            EXAMPLE_LCD_GPIO_DATA6,
            EXAMPLE_LCD_GPIO_DATA7,
            EXAMPLE_LCD_GPIO_DATA8,
            EXAMPLE_LCD_GPIO_DATA9,
            EXAMPLE_LCD_GPIO_DATA10,
            EXAMPLE_LCD_GPIO_DATA11,
            EXAMPLE_LCD_GPIO_DATA12,
            EXAMPLE_LCD_GPIO_DATA13,
            EXAMPLE_LCD_GPIO_DATA14,
            EXAMPLE_LCD_GPIO_DATA15,
        },
        .timings = {
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES,
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_back_porch = 40,
            .hsync_front_porch = 88,
            .hsync_pulse_width = 48,
            .vsync_back_porch = 26,
            .vsync_front_porch = 30,
            .vsync_pulse_width = 6,
            .flags.pclk_active_neg = true, // RGB data is clocked out on falling edge
        },
        .flags.fb_in_psram = true,
        .num_fbs = EXAMPLE_LCD_RGB_BUFFER_NUMS,
#if EXAMPLE_LCD_RGB_BOUNCE_BUFFER_MODE
        .bounce_buffer_size_px = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT,
#endif
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_rgb_panel(&panel_conf, &lcd_panel), err, TAG, "RGB init failed");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel));

    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(lcd_panel), err, TAG, "LCD init failed");

    return ret;

err:
    if (lcd_panel)
    {
        esp_lcd_panel_del(lcd_panel);
    }
    return ret;
}

esp_err_t lcd_clear_fast(esp_lcd_panel_handle_t panel, uint16_t color)
{
    uint16_t fact = 68;
    // uint64_t s = esp_timer_get_time();
    uint16_t *buffer = heap_caps_malloc(EXAMPLE_LCD_H_RES * fact * sizeof(uint16_t), MALLOC_CAP_SPIRAM); // MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return ESP_FAIL;
    }
    else
    {
        for (uint16_t i = 0; i < EXAMPLE_LCD_H_RES * fact; i++)
        {
            buffer[i] = color;
        }
        for (int y = 0; y < EXAMPLE_LCD_H_RES; y += fact)
        {
            esp_lcd_panel_draw_bitmap(panel, 0, y, EXAMPLE_LCD_H_RES, y + fact, buffer);
        }
        heap_caps_free(buffer);
    }
    // uint64_t t1 = (esp_timer_get_time() - s);
    // float time_per_frame = t1 / 1000;
    // float fps = 1000000.f / (float)t1;
    // ESP_LOGI(TAG, "@resolution %ux%u  [time per frame=%.2fMS, fps=%.2f]", LCD_WIDTH, LCD_HEIGHT, time_per_frame, fps);
    return ESP_OK;
}

static esp_err_t app_touch_init(void)
{
    /* Initilize I2C */
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_TOUCH_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = EXAMPLE_TOUCH_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = EXAMPLE_TOUCH_I2C_CLK_HZ};
    ESP_RETURN_ON_ERROR(i2c_param_config(EXAMPLE_TOUCH_I2C_NUM, &i2c_conf), TAG, "I2C configuration failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(EXAMPLE_TOUCH_I2C_NUM, i2c_conf.mode, 0, 0, 0), TAG, "I2C initialization failed");

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)EXAMPLE_TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
}

static esp_err_t app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 8192,       /* LVGL task stack size */
        .task_affinity = -1,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    uint32_t buff_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT;
#if EXAMPLE_LCD_LVGL_FULL_REFRESH || EXAMPLE_LCD_LVGL_DIRECT_MODE
    buff_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES;
#endif

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = lcd_panel,
        .buffer_size = buff_size,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
#if EXAMPLE_LCD_LVGL_FULL_REFRESH
            .full_refresh = true,
#elif EXAMPLE_LCD_LVGL_DIRECT_MODE
            .direct_mode = true,
#endif
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = false,
#endif
        }};
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
#if EXAMPLE_LCD_RGB_BOUNCE_BUFFER_MODE
            .bb_mode = true,
#else
            .bb_mode = false,
#endif
#if EXAMPLE_LCD_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        }};
    lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}

static void _app_button_cb(lv_event_t *e)
{
    lv_disp_rotation_t rotation = lv_disp_get_rotation(lvgl_disp);
    rotation++;
    if (rotation > LV_DISPLAY_ROTATION_270)
    {
        rotation = LV_DISPLAY_ROTATION_0;
    }

    /* LCD HW rotation */
    lv_disp_set_rotation(lvgl_disp, rotation);
}

static void app_main_display(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* Your LVGL objects code here .... */

    /* Create image */
    lv_obj_t *img_logo = lv_img_create(scr);
    lv_img_set_src(img_logo, LV_SYMBOL_OK);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 20);

    /* Label */
    lv_obj_t *label = lv_label_create(scr);
    lv_obj_set_width(label, EXAMPLE_LCD_H_RES);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
#if LVGL_VERSION_MAJOR == 8
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "#FF0000 " LV_SYMBOL_BELL " Hello world Espressif and LVGL " LV_SYMBOL_BELL "#\n#FF9400 " LV_SYMBOL_WARNING " For simplier initialization, use BSP " LV_SYMBOL_WARNING " #");
#else
    lv_label_set_text(label, LV_SYMBOL_BELL " Hello world Espressif and LVGL " LV_SYMBOL_BELL "\n " LV_SYMBOL_WARNING " For simplier initialization, use BSP " LV_SYMBOL_WARNING);
#endif
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);

    /* Button */
    lv_obj_t *btn = lv_btn_create(scr);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "Rotate screen");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn, _app_button_cb, LV_EVENT_CLICKED, NULL);
}

//---------------------------------------
void init_lcd_backlight_pwm(int gpio_num)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = BL_LEDC_MODE,
        .timer_num = BL_LEDC_TIMER,
        .duty_resolution = BL_LEDC_DUTY_RES,
        .freq_hz = BL_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = BL_LEDC_MODE,
        .channel = BL_LEDC_CHANNEL,
        .timer_sel = BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio_num,
        .duty = 255, // Zaczynamy od maksymalnej jasności (255)
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
}

//------------------------------------------
void set_lcd_brightness(uint32_t brightness)
{
    if (brightness > 255)
        brightness = 255;

    ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, brightness);
    ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
}
//-----------------
void app_main(void)
{

    // - set backlight PWM -
    init_lcd_backlight_pwm(2);
    set_lcd_brightness(100);

    /* LCD HW initialization */
    ESP_ERROR_CHECK(app_lcd_init());

    // lcd_clear_fast(lcd_panel, 0xF800);

    /* Touch initialization */
    ESP_ERROR_CHECK(app_touch_init());

    /* LVGL initialization */
    ESP_ERROR_CHECK(app_lvgl_init());

    /* Show LVGL objects */
    lvgl_port_lock(0);
    // app_main_display();
    lv_demo_benchmark();
    lvgl_port_unlock();
}