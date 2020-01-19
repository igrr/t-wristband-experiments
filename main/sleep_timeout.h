#pragma once

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SLEEP_EVENT);

/* Sleep event IDs */
enum {
    SLEEP_TIMEOUT
};

void sleep_timeout_init(int timeout_ms);
void sleep_timeout_reset(void);
