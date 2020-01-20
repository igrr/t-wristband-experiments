#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

void display_init(void);
void display_hello(void);
void display_time(const struct tm *tm);

#ifdef __cplusplus
}
#endif
