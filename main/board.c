/**
 *  T-Wristband board specific handling.
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "board.h"

static void board_touchpad_intr_handler(void *arg);
void board_lcd_init(void);

static int64_t s_touchpad_press_time;
static board_config_t s_config;

ESP_EVENT_DEFINE_BASE(BOARD_EVENT);

static const char *TAG = "board";

void board_init(const board_config_t *config)
{
    s_config = *config;
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
}

void board_touchpad_enable(void)
{
    gpio_config_t pwr_pin_config = {
        .pin_bit_mask = BIT64(TP_PWR_PIN),
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_ERROR_CHECK(gpio_set_level(TP_PWR_PIN, 1));
    ESP_ERROR_CHECK(gpio_config(&pwr_pin_config));
    ESP_ERROR_CHECK(gpio_hold_en(TP_PWR_PIN));


    gpio_config_t int_pin_config = {
        .pin_bit_mask = BIT64(TP_INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_PIN_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&int_pin_config));
    if (gpio_get_level(TP_INT_PIN)) {
        s_touchpad_press_time = esp_timer_get_time();
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(TP_INT_PIN, board_touchpad_intr_handler, NULL));
}

void board_sleep(void)
{
    esp_deep_sleep_disable_rom_logging();
    esp_sleep_enable_ext1_wakeup(BIT64(TP_INT_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
}

static void board_touchpad_intr_handler(void *arg)
{
    int task_unblocked = 0;
    int level = gpio_get_level(TP_INT_PIN);
    if (level) {
        ESP_EARLY_LOGI(TAG, "Touchpad press");
        s_touchpad_press_time = esp_timer_get_time();
    } else if (s_touchpad_press_time != 0) {
        int down_time_ms = (int) (esp_timer_get_time() - s_touchpad_press_time) / 1000;
        int event_id = (down_time_ms < s_config.touchpad_long_press_threshold_ms) ?
                       TOUCHPAD_PRESS : TOUCHPAD_LONG_PRESS;
        ESP_EARLY_LOGI(TAG, "Touchpad release, t=%dms, event=%d", down_time_ms, event_id);
        ESP_ERROR_CHECK(esp_event_isr_post(BOARD_EVENT, event_id, NULL, 0, &task_unblocked));
        s_touchpad_press_time = 0;
    }
    if (task_unblocked) {
        portYIELD_FROM_ISR();
    }
}

void board_lcd_enable(void)
{
    gpio_config_t pins_config = {
        .pin_bit_mask = BIT64(TFT_RST_PIN) | BIT64(TFT_BL_PIN) | BIT64(TFT_DC_PIN),
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_ERROR_CHECK(gpio_config(&pins_config));
    gpio_set_level(TFT_RST_PIN, 1);
}

void board_lcd_backlight(bool enable)
{
    gpio_set_level(TFT_BL_PIN, enable);
}
