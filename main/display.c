/**
 *  T-Wristband display handling.
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include <string.h>
#include <time.h>
#include "display.h"
#include "board.h"
#include "st7735.h"


void display_init(void)
{
    st7735_init();
}

static void draw_box(void)
{
    st7735_draw_line_h(0, MAX_X - 1, MIN_Y, 0x04af);
    st7735_draw_line_h(0, MAX_X - 1, MAX_Y - 1, 0x04af);
    st7735_draw_line_v(1, MIN_Y, MAX_Y - 1, 0x04af);
    st7735_draw_line_v(MAX_X - 1, MIN_Y, MAX_Y - 1, 0x04af);
}

void display_hello(void)
{
    st7735_clear_screen(0xffff);

    draw_box();

    st7735_set_position(10, MIN_Y + 10);
    st7735_draw_str("Hello", 0x007b, X3);
    st7735_set_position(10, MIN_Y + 45);
    st7735_draw_str("T-Wristband", 0x007b, X3);

    st7735_update_screen();
}

void display_time(const struct tm *tm)
{
    st7735_clear_screen(0xffff);
    draw_box();

    char buf[64] = {};
    strftime(buf, sizeof(buf), "%a", tm);
    st7735_set_position(10, MIN_Y + 8);
    st7735_draw_str(buf, 0x007b, X3);

    memset(buf, 0, sizeof(buf));
    strftime(buf, sizeof(buf), "%d", tm);
    st7735_set_position(18, MIN_Y + 32);
    st7735_draw_str(buf, 0x007b, X3);

    memset(buf, 0, sizeof(buf));
    strftime(buf, sizeof(buf), "%b", tm);
    st7735_set_position(10, MIN_Y + 56);
    st7735_draw_str(buf, 0x007b, X3);

    memset(buf, 0, sizeof(buf));
    strftime(buf, sizeof(buf), "%H:%M", tm);
    st7735_set_position(75, MIN_Y + 32);
    st7735_draw_str(buf, 0x007b, X3);

    st7735_update_screen();
}
