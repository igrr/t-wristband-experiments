/** 
 * ST7735 LCD driver, as an ESP-IDF component
 *
 * Copyright (C) 2016 Marian Hrinko.
 * Written by Marian Hrinko (mato.hrinko@gmail.com)
 * Original repostory: https://github.com/Matiasus/ST7735.
 *
 * Adaptations to ESP-IDF (c) 2020 Ivan Grokhotkov
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "st7735_defs.h"

void st7735_init(void);

uint8_t st7735_set_window(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1);
bool st7735_set_position(uint8_t x, uint8_t y);

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
char st7735_draw_char(char c, uint16_t color, ESizes size);
void st7735_draw_str(const char* str, uint16_t color, ESizes size);

void st7735_draw_line(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint16_t color);
void st7735_draw_line_h(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color);
void st7735_draw_line_v(uint8_t x, uint8_t y0, uint8_t y1, uint16_t color);

void st7735_clear_screen(uint16_t color);
void st7735_update_screen(void);


#ifdef __cplusplus
}
#endif
