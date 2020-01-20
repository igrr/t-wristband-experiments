#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void pcf8563_init(int i2c_port);
void pcf8563_get_time(struct tm *out);
void pcf8563_set_time(const struct tm *in);

#ifdef __cplusplus
}
#endif
