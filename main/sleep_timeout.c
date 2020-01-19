/**
 *  T-Wristband sleep timeout logic.
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "sleep_timeout.h"
#include "board.h"
#include "sys/lock.h"

static void sleep_timeout_cb(void* arg);

static esp_timer_handle_t s_sleep_timer;
static int s_sleep_timeout_ms;

ESP_EVENT_DEFINE_BASE(SLEEP_EVENT);
static const char* TAG = "sleep";

void sleep_timeout_init(int timeout_ms)
{
    s_sleep_timeout_ms = timeout_ms;

    esp_timer_create_args_t args = {
        .callback = &sleep_timeout_cb,
        .name = "sleep_timeout"
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_sleep_timer));
    sleep_timeout_reset();
}

static void sleep_timeout_cb(void* arg)
{
    ESP_ERROR_CHECK(esp_event_post(SLEEP_EVENT, SLEEP_TIMEOUT, NULL, 0, portMAX_DELAY));
}

void sleep_timeout_reset(void)
{
    static _lock_t lock;
    _lock_acquire(&lock);
    assert(s_sleep_timer != NULL);
    esp_timer_stop(s_sleep_timer);
    ESP_LOGI(TAG, "starting sleep timer");
    ESP_ERROR_CHECK(esp_timer_start_once(s_sleep_timer, s_sleep_timeout_ms * 1000));
    _lock_release(&lock);
}
