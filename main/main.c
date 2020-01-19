/**
 *  T-Wristband main application code.
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_event.h"
#include "board.h"
#include "sleep_timeout.h"
#include "display.h"

#define SLEEP_TIMEOUT_MS 3000

#define EVENT_HANDLER(name_) void name_(void* arg, esp_event_base_t base, int id, void* data)

static void register_handlers(void);
static EVENT_HANDLER(on_sleep_timeout);
static EVENT_HANDLER(on_touchpad_press);
static EVENT_HANDLER(on_touchpad_long_press);


static const char* TAG = "main";

void app_main(void)
{
    esp_event_loop_create_default();
    register_handlers();

    board_config_t board_config = BOARD_CONFIG_DEFAULT();
    board_init(&board_config);
    board_touchpad_enable();
    board_lcd_enable();

    display_init();
    display_hello();

    sleep_timeout_init(SLEEP_TIMEOUT_MS);
}

static void register_handlers(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(SLEEP_EVENT, SLEEP_TIMEOUT, &on_sleep_timeout, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BOARD_EVENT, TOUCHPAD_PRESS, &on_touchpad_press, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(BOARD_EVENT, TOUCHPAD_LONG_PRESS, &on_touchpad_long_press, NULL));
}

static EVENT_HANDLER(on_sleep_timeout)
{
    ESP_LOGI(TAG, "Entering sleep");
    fflush(stdout);
    fsync(fileno(stdout));

    board_sleep();
}

static EVENT_HANDLER(on_touchpad_press)
{
    ESP_LOGI(TAG, "Touchpad press");
    sleep_timeout_reset();
}

static EVENT_HANDLER(on_touchpad_long_press)
{
    ESP_LOGI(TAG, "Touchpad long press");
    sleep_timeout_reset();
}
