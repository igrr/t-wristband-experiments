/**
 *  Minimal PCF8563 driver
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include <time.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "pcf8563.h"

#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

#define PCF8563_CTRL1_REG  0x00
#define PCF8563_CTRL2_REG  0x01
#define PCF8563_SEC_REG  0x02
#define PCF8563_SEC_MASK 0x7f
#define PCF8563_MIN_REG  0x03
#define PCF8563_MIN_MASK 0x7f
#define PCF8563_HR_REG  0x04
#define PCF8563_HR_MASK 0x3f
#define PCF8563_DAY_REG  0x05       /* 1 to 31 */
#define PCF8563_DAY_MASK 0x3f
#define PCF8563_WKDAY_REG  0x06     /* 0 to 6 */
#define PCF8563_WKDAY_MASK 0x07
#define PCF8563_MON_REG  0x07       /* 1 to 12 */
#define PCF8563_MON_MASK 0x1f
#define PCF8563_YEAR_REG  0x08      /* 0 to 99 */
#define PCF8563_YEAR_MASK 0xff


typedef struct {
    union {
        struct {
            uint8_t low: 4;
            uint8_t high: 4;
        };
        uint8_t val;
    };
} bcd_t;

typedef struct {
    union {
        bcd_t sec;
        struct {
            uint8_t unused1: 7;
            uint8_t valid: 1;
        };
    };
    bcd_t min;
    bcd_t hr;
    bcd_t day;
    bcd_t wkday;
    union {
        bcd_t mon;
        struct {
            uint8_t unused2: 7;
            uint8_t century: 1;
        };
    };
    bcd_t yr;
} pcf8563_datetime_t;

_Static_assert(sizeof(pcf8563_datetime_t) == PCF8563_YEAR_REG - PCF8563_SEC_REG + 1,
               "incorrect size of pcf8563_datetime_t");


static esp_err_t pcf8563_read(uint8_t reg, uint8_t *result, size_t len);
static esp_err_t pcf8563_write(uint8_t reg, const uint8_t *data, size_t len);
static void pcf8563_datetime_to_tm(const pcf8563_datetime_t *in, struct tm *out);
static void pcf8563_tm_to_datetime(const struct tm *in, pcf8563_datetime_t *out);
static int bcd_to_int(bcd_t val, uint8_t mask);
static bcd_t int_to_bcd(int val);
static void log_tm(const char *comment, const struct tm *in);


static i2c_port_t s_i2c_port;
const uint8_t s_slave_addr = 0x51;
static const char *TAG = "pcf8563";

void pcf8563_init(int i2c_port)
{
    s_i2c_port = i2c_port;
    uint8_t val;
    esp_err_t err = pcf8563_read(PCF8563_CTRL1_REG, &val, 1);
    ESP_ERROR_CHECK(err);
}

void pcf8563_get_time(struct tm *out)
{
    pcf8563_datetime_t datetime;
    esp_err_t err = pcf8563_read(PCF8563_SEC_REG, (uint8_t *) &datetime, sizeof(datetime));
    ESP_LOG_BUFFER_HEX(TAG, &datetime, sizeof(datetime));
    ESP_ERROR_CHECK(err);
    pcf8563_datetime_to_tm(&datetime, out);
    log_tm(__func__, out);
}

void pcf8563_set_time(const struct tm *in)
{
    pcf8563_datetime_t datetime;
    pcf8563_tm_to_datetime(in, &datetime);
    log_tm(__func__, in);
    ESP_LOG_BUFFER_HEX(TAG, &datetime, sizeof(datetime));
    esp_err_t err = pcf8563_write(PCF8563_SEC_REG, (const uint8_t *) &datetime, sizeof(datetime));
    ESP_ERROR_CHECK(err);
}

static esp_err_t pcf8563_read(uint8_t reg, uint8_t *result, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, s_slave_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, s_slave_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    if (len > 1) {
        i2c_master_read(cmd, result, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, result + len - 1, NACK_VAL);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t pcf8563_write(uint8_t reg, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, s_slave_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    for (size_t i = 0; i < len; i++) {
        i2c_master_write_byte(cmd, data[i], ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void pcf8563_datetime_to_tm(const pcf8563_datetime_t *in, struct tm *out)
{
    unsigned century = in->century;
    *out = (struct tm) {
        .tm_sec = bcd_to_int(in->sec, PCF8563_SEC_MASK),
        .tm_min = bcd_to_int(in->min, PCF8563_MIN_MASK),
        .tm_hour = bcd_to_int(in->hr, PCF8563_HR_MASK),
        .tm_mday = bcd_to_int(in->day, PCF8563_DAY_MASK),
        .tm_mon = bcd_to_int(in->mon, PCF8563_MON_MASK) - 1,
        .tm_year = bcd_to_int(in->yr, PCF8563_YEAR_MASK) + (century + 1) * 100,
        .tm_wday = bcd_to_int(in->wkday, PCF8563_WKDAY_MASK)
    };
}

static void pcf8563_tm_to_datetime(const struct tm *in, pcf8563_datetime_t *out)
{
    *out = (pcf8563_datetime_t) {
        .sec = int_to_bcd(in->tm_sec),
        .min = int_to_bcd(in->tm_min),
        .hr = int_to_bcd(in->tm_hour),
        .day = int_to_bcd(in->tm_mday),
        .mon = int_to_bcd(in->tm_mon + 1),
        .yr = int_to_bcd(in->tm_year % 100),
        .wkday = int_to_bcd(in->tm_wday)
    };
    out->century = in->tm_year / 100 - 1;
}

static int bcd_to_int(bcd_t val, uint8_t mask)
{
    bcd_t tmp = {
        .val = val.val & mask
    };
    return tmp.low + 10 * tmp.high;
}

static bcd_t int_to_bcd(int val)
{
    assert(val < 100);

    return (bcd_t) {
        .low = val % 10,
        .high = val / 10
    };
}

static void log_tm(const char *comment, const struct tm *in)
{
    char buf[64];
    if (strftime(buf, sizeof(buf), "%A %c", in)) {
        ESP_LOGI(TAG, "%s: %s", comment, buf);
    }
}