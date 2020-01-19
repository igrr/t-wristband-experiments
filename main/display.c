/**
 *  T-Wristband display handling.
 *
 *  Copyright (c) 2020 Ivan Grokhotkov
 *  Distributed under MIT license as displayed in LICENSE file.
 */

#include "display.h"
#include "board.h"
#include "st7735.h"


void display_init(void)
{
    st7735_init();
}

void display_hello(void)
{
    st7735_draw_line_h(0, MAX_X - 1, MIN_Y, 0x04af);
    st7735_draw_line_h(0, MAX_X - 1, MAX_Y - 1, 0x04af);
    st7735_draw_line_v(1, MIN_Y, MAX_Y - 1, 0x04af);
    st7735_draw_line_v(MAX_X - 1, MIN_Y, MAX_Y - 1, 0x04af);
    st7735_update_screen();

    st7735_set_position(10, MIN_Y + 10);
    st7735_draw_str("Hello", 0x007b, X3);
    st7735_set_position(10, MIN_Y + 45);
    st7735_draw_str("T-Wristband", 0x007b, X3);

    st7735_update_screen();

    /* only turn on the backlight when finished drawing */
    board_lcd_backlight(true);
}
