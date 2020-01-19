/** 
 * ST7735 LCD driver, as an ESP-IDF component
 *
 * Copyright (C) 2016 Marian Hrinko.
 * Written by Marian Hrinko (mato.hrinko@gmail.com)
 * Original repostory: https://github.com/Matiasus/ST7735.
 *
 * Adaptations to ESP-IDF (c) 2020 Ivan Grokhotkov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "st7735.h"
#include "st7735_defs.h"

static void st7735_commands(const uint8_t *commands);

static void st7735_send_command(uint8_t);
static void st7735_send_data8(uint8_t);
static void st7735_send_data16(uint16_t);
static void st7735_delay_ms(uint8_t);
static void st7735_fill_color565(uint16_t color, uint16_t count);

static void st7735_logical_to_lcd(int *x, int* y);

/** @array Init command */
static const uint8_t st7735_init_commands[] = {
    // 11 commands in list:
    11,
    // Software reset
    //  no arguments
    //  delay
    SWRESET, 
        DELAY,  
          200,  // 200 ms delay
    // Out of sleep mode, 
    //  no arguments, 
    //  delay
    SLPOUT,
        DELAY,  
          200,  // 200 ms delay
    // Set color mode, 
    //  1 argument
    //  delay
    COLMOD, 
      1+DELAY,  
         0x05,  // 16-bit color
           10,  // 10 ms
    // Frame rate control, 
    //  3 arguments
    //  delay
    FRMCTR1,
      3+DELAY,  
         0x00,  // fastest refresh
         0x06,  // 6 lines front porch
         0x03,  // 3 lines back porch
           10,  // 10 ms delay
    // Inversion mode off
     INVOFF,
        DELAY,  
           10,
    // Memory access ctrl (directions), 
    //  1 argument
    //  no delay
     MADCTL, 
            1,
         // D7  D6  D5  D4  D3  D2  D1  D0
         // MY  MX  MV  ML RGB  MH   -   -
         // ------------------------------
         // ------------------------------
         // MV  MX  MY -> {MV (row / column exchange) MX (column address order), MY (row address order)}
         // ------------------------------
         //  0   0   0 -> begin left-up corner, end right-down corner 
         //               left-right (normal view) 
         //  0   0   1 -> begin left-down corner, end right-up corner 
         //               left-right (Y-mirror)
         //  0   1   0 -> begin right-up corner, end left-down corner 
         //               right-left (X-mirror)
         //  0   1   1 -> begin right-down corner, end left-up corner
         //               right-left (X-mirror, Y-mirror)
         //  1   0   0 -> begin left-up corner, end right-down corner
         //               up-down (X-Y exchange)  
         //  1   0   1 -> begin left-down corner, end right-up corner
         //               down-up (X-Y exchange, Y-mirror)
         //  1   1   0 -> begin right-up corner, end left-down corner 
         //               up-down (X-Y exchange, X-mirror)  
         //  1   1   1 -> begin right-down corner, end left-up corner
         //               down-up (X-Y exchange, X-mirror, Y-mirror)
         // ------------------------------
         //  ML: vertical refresh order 
         //      0 -> refresh top to bottom 
         //      1 -> refresh bottom to top
         // ------------------------------
         // RGB: filter panel
         //      0 -> RGB 
         //      1 -> BGR        
         // ------------------------------ 
         //  MH: horizontal refresh order 
         //      0 -> refresh left to right 
         //      1 -> refresh right to left
         // 0xA0 = 1010 0000  
         (BIT(7) | BIT(5) | BIT(3)),
    // Display settings #5, 
    //  2 arguments 
    //  no delay
    DISSET5, 
            2,  
         0x15,  // 1 clk cycle nonoverlap, 2 cycle gate
                // rise, 3 cycle osc equalize
         0x02,  // Fix on VTL
    // Display inversion control, 
    //  1 argument
    //  no delay
     INVCTR,
            1,  
          0x0,  //     Line inversion
    // Magical unicorn dust, 
    //  16 arguments
    //  no delay
    GMCTRP1,
           16,
         0x09, 
         0x16, 
         0x09, 
         0x20,
         0x21, 
         0x1B,
         0x13,
         0x19,
         0x17,
         0x15,
         0x1E,
         0x2B,
         0x04,
         0x05,
         0x02,
         0x0E,
    // Sparkles and rainbows 
    //  16 arguments 
    //  delay
    GMCTRN1,
     16+DELAY,  
         0x0B,
         0x14,
         0x08,
         0x1E, 
         0x22,
         0x1D,
         0x18,
         0x1E,
         0x1B,
         0x1A,
         0x24,  
         0x2B,
         0x06,
         0x06,
         0x02,
         0x0F,
           10,  // 10 ms delay
    // Normal display on
    //  no arguments
    //  delay
      NORON, 
        DELAY, 
           10,  // 10 ms delay
/*
    // Main screen turn on
    //  no arguments
    //  delay
     DISPON,
        DELAY,  
          200   // 200 ms delay
*/
};
/** @array Charset */
const uint8_t CHARACTERS[][5] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00 }, // 20 space
  { 0x00, 0x00, 0x5f, 0x00, 0x00 }, // 21 !
  { 0x00, 0x07, 0x00, 0x07, 0x00 }, // 22 "
  { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, // 23 #
  { 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, // 24 $
  { 0x23, 0x13, 0x08, 0x64, 0x62 }, // 25 %
  { 0x36, 0x49, 0x55, 0x22, 0x50 }, // 26 &
  { 0x00, 0x05, 0x03, 0x00, 0x00 }, // 27 '
  { 0x00, 0x1c, 0x22, 0x41, 0x00 }, // 28 (
  { 0x00, 0x41, 0x22, 0x1c, 0x00 }, // 29 )
  { 0x14, 0x08, 0x3e, 0x08, 0x14 }, // 2a *
  { 0x08, 0x08, 0x3e, 0x08, 0x08 }, // 2b +
  { 0x00, 0x50, 0x30, 0x00, 0x00 }, // 2c ,
  { 0x08, 0x08, 0x08, 0x08, 0x08 }, // 2d -
  { 0x00, 0x60, 0x60, 0x00, 0x00 }, // 2e .
  { 0x20, 0x10, 0x08, 0x04, 0x02 }, // 2f /
  { 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 30 0
  { 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 31 1
  { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 32 2
  { 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 33 3
  { 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 34 4
  { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 35 5
  { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 36 6
  { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 37 7
  { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 38 8
  { 0x06, 0x49, 0x49, 0x29, 0x1e }, // 39 9
  { 0x00, 0x36, 0x36, 0x00, 0x00 }, // 3a :
  { 0x00, 0x56, 0x36, 0x00, 0x00 }, // 3b ;
  { 0x08, 0x14, 0x22, 0x41, 0x00 }, // 3c <
  { 0x14, 0x14, 0x14, 0x14, 0x14 }, // 3d =
  { 0x00, 0x41, 0x22, 0x14, 0x08 }, // 3e >
  { 0x02, 0x01, 0x51, 0x09, 0x06 }, // 3f ?
  { 0x32, 0x49, 0x79, 0x41, 0x3e }, // 40 @
  { 0x7e, 0x11, 0x11, 0x11, 0x7e }, // 41 A
  { 0x7f, 0x49, 0x49, 0x49, 0x36 }, // 42 B
  { 0x3e, 0x41, 0x41, 0x41, 0x22 }, // 43 C
  { 0x7f, 0x41, 0x41, 0x22, 0x1c }, // 44 D
  { 0x7f, 0x49, 0x49, 0x49, 0x41 }, // 45 E
  { 0x7f, 0x09, 0x09, 0x09, 0x01 }, // 46 F
  { 0x3e, 0x41, 0x49, 0x49, 0x7a }, // 47 G
  { 0x7f, 0x08, 0x08, 0x08, 0x7f }, // 48 H
  { 0x00, 0x41, 0x7f, 0x41, 0x00 }, // 49 I
  { 0x20, 0x40, 0x41, 0x3f, 0x01 }, // 4a J
  { 0x7f, 0x08, 0x14, 0x22, 0x41 }, // 4b K
  { 0x7f, 0x40, 0x40, 0x40, 0x40 }, // 4c L
  { 0x7f, 0x02, 0x0c, 0x02, 0x7f }, // 4d M
  { 0x7f, 0x04, 0x08, 0x10, 0x7f }, // 4e N
  { 0x3e, 0x41, 0x41, 0x41, 0x3e }, // 4f O
  { 0x7f, 0x09, 0x09, 0x09, 0x06 }, // 50 P
  { 0x3e, 0x41, 0x51, 0x21, 0x5e }, // 51 Q
  { 0x7f, 0x09, 0x19, 0x29, 0x46 }, // 52 R
  { 0x46, 0x49, 0x49, 0x49, 0x31 }, // 53 S
  { 0x01, 0x01, 0x7f, 0x01, 0x01 }, // 54 T
  { 0x3f, 0x40, 0x40, 0x40, 0x3f }, // 55 U
  { 0x1f, 0x20, 0x40, 0x20, 0x1f }, // 56 V
  { 0x3f, 0x40, 0x38, 0x40, 0x3f }, // 57 W
  { 0x63, 0x14, 0x08, 0x14, 0x63 }, // 58 X
  { 0x07, 0x08, 0x70, 0x08, 0x07 }, // 59 Y
  { 0x61, 0x51, 0x49, 0x45, 0x43 }, // 5a Z
  { 0x00, 0x7f, 0x41, 0x41, 0x00 }, // 5b [
  { 0x02, 0x04, 0x08, 0x10, 0x20 }, // 5c backslash
  { 0x00, 0x41, 0x41, 0x7f, 0x00 }, // 5d ]
  { 0x04, 0x02, 0x01, 0x02, 0x04 }, // 5e ^
  { 0x40, 0x40, 0x40, 0x40, 0x40 }, // 5f _
  { 0x00, 0x01, 0x02, 0x04, 0x00 }, // 60 `
  { 0x20, 0x54, 0x54, 0x54, 0x78 }, // 61 a
  { 0x7f, 0x48, 0x44, 0x44, 0x38 }, // 62 b
  { 0x38, 0x44, 0x44, 0x44, 0x20 }, // 63 c
  { 0x38, 0x44, 0x44, 0x48, 0x7f }, // 64 d
  { 0x38, 0x54, 0x54, 0x54, 0x18 }, // 65 e
  { 0x08, 0x7e, 0x09, 0x01, 0x02 }, // 66 f
  { 0x0c, 0x52, 0x52, 0x52, 0x3e }, // 67 g
  { 0x7f, 0x08, 0x04, 0x04, 0x78 }, // 68 h
  { 0x00, 0x44, 0x7d, 0x40, 0x00 }, // 69 i
  { 0x20, 0x40, 0x44, 0x3d, 0x00 }, // 6a j
  { 0x7f, 0x10, 0x28, 0x44, 0x00 }, // 6b k
  { 0x00, 0x41, 0x7f, 0x40, 0x00 }, // 6c l
  { 0x7c, 0x04, 0x18, 0x04, 0x78 }, // 6d m
  { 0x7c, 0x08, 0x04, 0x04, 0x78 }, // 6e n
  { 0x38, 0x44, 0x44, 0x44, 0x38 }, // 6f o
  { 0x7c, 0x14, 0x14, 0x14, 0x08 }, // 70 p
  { 0x08, 0x14, 0x14, 0x14, 0x7c }, // 71 q
  { 0x7c, 0x08, 0x04, 0x04, 0x08 }, // 72 r
  { 0x48, 0x54, 0x54, 0x54, 0x20 }, // 73 s
  { 0x04, 0x3f, 0x44, 0x40, 0x20 }, // 74 t
  { 0x3c, 0x40, 0x40, 0x20, 0x7c }, // 75 u
  { 0x1c, 0x20, 0x40, 0x20, 0x1c }, // 76 v
  { 0x3c, 0x40, 0x30, 0x40, 0x3c }, // 77 w
  { 0x44, 0x28, 0x10, 0x28, 0x44 }, // 78 x
  { 0x0c, 0x50, 0x50, 0x50, 0x3c }, // 79 y
  { 0x44, 0x64, 0x54, 0x4c, 0x44 }, // 7a z
  { 0x00, 0x08, 0x36, 0x41, 0x00 }, // 7b {
  { 0x00, 0x00, 0x7f, 0x00, 0x00 }, // 7c |
  { 0x00, 0x41, 0x36, 0x08, 0x00 }, // 7d }
  { 0x10, 0x08, 0x08, 0x10, 0x08 }, // 7e ~
  { 0x00, 0x00, 0x00, 0x00, 0x00 }  // 7f
};

int s_cur_y = 0;
int s_cur_x = 0;

static spi_device_handle_t s_spi_dev;

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(TFT_DC, dc);
}

static void st7735_spi_init(void)
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=TFT_MOSI,
        .sclk_io_num=TFT_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=2*320*2+8
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=TFT_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 0);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &s_spi_dev);
    ESP_ERROR_CHECK(ret);
}

void st7735_init(void)
{
  st7735_spi_init();
  // load list of commands
  st7735_commands(st7735_init_commands);
}

static void st7735_commands(const uint8_t *commands)
{
  uint8_t milliseconds;
  uint8_t numOfCommands;
  uint8_t numOfArguments;

  // number of commands
  numOfCommands = *(commands++);
  
  // loop through whole command list
  while (numOfCommands--) {
    // send command
    st7735_send_command(*(commands++));
    // read number of arguments
    numOfArguments = *(commands++);
    // check if delay set
    milliseconds = numOfArguments & DELAY;
    // remove delay flag
    numOfArguments &= ~DELAY;
    // loop through number of arguments
    while (numOfArguments--) {
      // send arguments
      st7735_send_data8(*(commands++));
    }
    // check if delay set
    if (milliseconds) {
      // value in milliseconds
      milliseconds = *(commands++);
      // delay
      st7735_delay_ms(milliseconds);
    }
  }
}


static void st7735_send_command(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t = {};
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&data;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(s_spi_dev, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

static void st7735_send_data8(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t = {};
    t.length=8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=&data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(s_spi_dev, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

static void st7735_send_data16(uint16_t data)
{
    esp_err_t ret;
    spi_transaction_t t = {};
    t.length=16;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=&data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(s_spi_dev, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

static void st7735_fill_color565(uint16_t color, uint16_t count)
{
  // access to RAM
  st7735_send_command(RAMWR);
  // counter
  while (count--) {
    // write color
    st7735_send_data16(color);
  }
}

uint8_t st7735_set_window(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1)
{
  // check if coordinates is out of range
  if ((x0 > x1)     ||
      (x1 > MAX_X) ||
      (y0 > y1)     ||
      (y1 > MAX_Y)) { 
    // out of range
    return 0;
  }  
  // column address set
  st7735_send_command(CASET);
  // start x position
  st7735_send_data8(0x00);
  // start x position
  st7735_send_data8(x0);
  // start x position
  st7735_send_data8(0x00);
  // end x position
  st7735_send_data8(x1);
  // row address set
  st7735_send_command(RASET);
  // start x position
  st7735_send_data8(0x00);
  // start y position
  st7735_send_data8(y0);
  // start x position
  st7735_send_data8(0x00);
  // end y position
  st7735_send_data8(y1);
  // success
  return 1;
}

bool st7735_set_position(uint8_t x, uint8_t y)
{
  // check if coordinates is out of range
  if ((x > MAX_X - (CHARS_COLS_LEN + 1)) &&
      (y > MAX_Y - (CHARS_ROWS_LEN))) {
    // out of range
    return false;
  }
  // check if x coordinates is out of range
  // and y is not out of range go to next line
  if ((x > MAX_X - (CHARS_COLS_LEN + 1)) &&
      (y < MAX_Y - (CHARS_ROWS_LEN))) {
    // change position y
    s_cur_y = y + CHARS_ROWS_LEN;
    // change position x
    s_cur_x = x;
  } else {
    // set position y 
    s_cur_y = y;
    // set position x
    s_cur_x = x;
  }
  // success
  return true;
}


void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
  // set window
  st7735_set_window(x, x, y, y);
  // draw pixel by 565 mode
  st7735_fill_color565(color, 1);
}


char st7735_draw_char(char character, uint16_t color, ESizes size)
{
  // variables
  uint8_t letter, idxCol, idxRow;
  // check if character is out of range
  if ((character < 0x20) &&
      (character > 0x7f)) { 
    // out of range
    return 0;
  }
  // last column of character array - 5 columns 
  idxCol = CHARS_COLS_LEN;
  // last row of character array - 8 rows / bits
  idxRow = CHARS_ROWS_LEN;

  // ----------------------------------------
  // SIZE X1 - normal font: 1x high, 1x wide
  // ----------------------------------------
  if (size == X1) {  
    // loop through 5 bytes
    while (idxCol--) {
      // read from ROM memory 
      letter = *(&CHARACTERS[character - 32][idxCol]);
      // loop through 8 bits
      while (idxRow--) {
        // check if bit set
        if ((letter & 0x80) == 0x80) {
          // draw pixel 
          st7735_draw_pixel(s_cur_x + idxCol, s_cur_y + idxRow, color);
        }
        // byte move to left / next bit
        letter = letter << 1;
      }
      // fill index row again
      idxRow = CHARS_ROWS_LEN;
    }
  // -----------------------------------------
  // SIZE X2 - bigger font 2x higher, 1x wide
  // -----------------------------------------
  } else if (size == X2) {
    // loop through 5 bytes
    while (idxCol--) {
      // read from ROM memory 
      letter = *(&CHARACTERS[character - 32][idxCol]);
      // loop through 8 bits
      while (idxRow--) {
        // check if bit set
        if ((letter & 0x80) == 0x80) {
          // draw first up pixel; 
          // note: (idxRow << 1) - 2x multiplied 
          st7735_draw_pixel(s_cur_x + idxCol, s_cur_y + (idxRow << 1), color);
          // draw second down pixel
          st7735_draw_pixel(s_cur_x + idxCol, s_cur_y + (idxRow << 1) + 1, color);
        }
        // byte move to left / next bit
        letter = letter << 1;
      }
      // fill index row again
      idxRow = CHARS_ROWS_LEN;
    }
  // ------------------------------------------------
  // SIZE X3 - the biggest font: 2x higher, 2x wider
  // ------------------------------------------------
  } else if (size == X3) {
    // loop through 5 bytes
    while (idxCol--) {
      // read from ROM memory 
      letter = *(&CHARACTERS[character - 32][idxCol]);
      // loop through 8 bits
      while (idxRow--) {
        // check if bit set
        if ((letter & 0x80) == 0x80) {
          // draw first left up pixel; 
          // note: (idxRow << 1) - 2x multiplied 
          st7735_draw_pixel(s_cur_x + (idxCol << 1), s_cur_y + (idxRow << 1), color);
          // draw second left down pixel
          st7735_draw_pixel(s_cur_x + (idxCol << 1), s_cur_y + (idxRow << 1) + 1, color);
          // draw third right up pixel
          st7735_draw_pixel(s_cur_x + (idxCol << 1) + 1, s_cur_y + (idxRow << 1), color);
          // draw fourth right down pixel
          st7735_draw_pixel(s_cur_x + (idxCol << 1) + 1, s_cur_y + (idxRow << 1) + 1, color);
        }
        // byte move to left / next bit
        letter = letter << 1;
      }
      // fill index row again
      idxRow = CHARS_ROWS_LEN;
    }
  }
  // return exit
  return 0;
}

void st7735_draw_str(const char *str, uint16_t color, ESizes size)
{
  // variables
  uint8_t i = 0;
  // loop through character of string
  while (str[i] != '\0') {
    //read characters and increment index
    st7735_draw_char(str[i++], color, size);
    // update position
    st7735_set_position(s_cur_x + (CHARS_COLS_LEN + 1) + (size >> 1), s_cur_y);
  }
}

void st7735_draw_line(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint16_t color)
{
  // determinant
  int16_t D;
  // deltas
  int16_t delta_x, delta_y;
  // steps
  int16_t trace_x = 1, trace_y = 1;

  // delta x
  delta_x = x2 - x1;
  // delta y
  delta_y = y2 - y1;

  // check if x2 > x1
  if (delta_x < 0) {
    // negate delta x
    delta_x = -delta_x;
    // negate step x
    trace_x = -trace_x;
  }

  // check if y2 > y1
  if (delta_y < 0) {
    // negate detla y
    delta_y = -delta_y;
    // negate step y
    trace_y = -trace_y;
  }

  // Bresenham condition for m < 1 (dy < dx)
  if (delta_y < delta_x) {
    // calculate determinant
    D = (delta_y << 1) - delta_x;
    // draw first pixel
    st7735_draw_pixel(x1, y1, color);
    // check if x1 equal x2
    while (x1 != x2) {
      // update x1
      x1 += trace_x;
      // check if determinant is positive
      if (D >= 0) {
        // update y1
        y1 += trace_y;
        // update determinant
        D -= (delta_x << 1);    
      }
      // update deteminant
      D += (delta_y << 1);
      // draw next pixel
      st7735_draw_pixel(x1, y1, color);
    }
  // for m > 1 (dy > dx)    
  } else {
    // calculate determinant
    D = delta_y - (delta_x << 1);
    // draw first pixel
    st7735_draw_pixel(x1, y1, color);
    // check if y2 equal y1
    while (y1 != y2) {
      // update y1
      y1 += trace_y;
      // check if determinant is positive
      if (D <= 0) {
        // update y1
        x1 += trace_x;
        // update determinant
        D += (delta_y << 1);    
      }
      // update deteminant
      D -= (delta_x << 1);
      // draw next pixel
      st7735_draw_pixel(x1, y1, color);
    }
  }
}

void st7735_draw_line_h(uint8_t xs, uint8_t xe, uint8_t y, uint16_t color)
{
  uint8_t temp;
  // check if start is > as end  
  if (xs > xe) {
    // temporary safe
    temp = xe;
    // start change for end
    xe = xs;
    // end change for start
    xs = temp;
  }
  // set window
  st7735_set_window(xs, xe, y, y);
  // draw pixel by 565 mode
  st7735_fill_color565(color, xe - xs);
}

void st7735_draw_line_v(uint8_t x, uint8_t ys, uint8_t ye, uint16_t color)
{
  uint8_t temp;
  // check if start is > as end
  if (ys > ye) {
    // temporary safe
    temp = ye;
    // start change for end
    ye = ys;
    // end change for start
    ys = temp;
  }
  // set window
  st7735_set_window(x, x, ys, ye);
  // draw pixel by 565 mode
  st7735_fill_color565(color, ye - ys);
}

void st7735_clear_screen(uint16_t color)
{
  // set whole window
  st7735_set_window(0, MAX_X, MIN_Y, MAX_Y);
  // draw individual pixels 
  // CACHE_SIZE_MEM = SIZE_X * SIZE_Y
  st7735_fill_color565(color, CACHE_SIZE_MEM);
}

void st7735_update_screen(void)
{
  // display on
  st7735_send_command(DISPON);
}

static void st7735_delay_ms(uint8_t ms)
{
  usleep(100*ms); /* 10 times shorter delays also seem to work */
}
