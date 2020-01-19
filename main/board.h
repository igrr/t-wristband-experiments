#pragma once

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TP_INT_PIN          33
#define TP_PWR_PIN          25
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define IMU_INT_PIN         38
#define RTC_INT_PIN         34
#define BATT_ADC_PIN        35
#define VBUS_PIN            36
#define LED_PIN             4
#define CHARGE_PIN          32

typedef struct {
    int touchpad_long_press_threshold_ms;
} board_config_t;

#define BOARD_CONFIG_DEFAULT() (board_config_t) { \
    .touchpad_long_press_threshold_ms = 1500, \
};

void board_init(const board_config_t* config);
void board_touchpad_enable(void);
void board_sleep(void);

ESP_EVENT_DECLARE_BASE(BOARD_EVENT);

/* board event IDs */
enum {
    TOUCHPAD_PRESS,
    TOUCHPAD_LONG_PRESS
};


#ifdef __cplusplus
}
#endif
