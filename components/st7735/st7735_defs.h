#pragma once


#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   23
#define TFT_RST  26
#define TFT_BL   27


#define DELAY   0x80

#define NOP     0x00
#define SWRESET 0x01
#define RDDID   0x04
#define RDDST   0x09

#define SLPIN   0x10
#define SLPOUT  0x11
#define PTLON   0x12
#define NORON   0x13

#define INVOFF  0x20
#define INVON   0x21
#define DISPOFF 0x28
#define DISPON  0x29
#define RAMRD   0x2E
#define CASET   0x2A
#define RASET   0x2B
#define RAMWR   0x2C

#define PTLAR   0x30
#define MADCTL  0x36
#define COLMOD  0x3A

#define FRMCTR1 0xB1
#define FRMCTR2 0xB2
#define FRMCTR3 0xB3
#define INVCTR  0xB4
#define DISSET5 0xB6

#define PWCTR1  0xC0
#define PWCTR2  0xC1
#define PWCTR3  0xC2
#define PWCTR4  0xC3
#define PWCTR5  0xC4
#define VMCTR1  0xC5

#define RDID1   0xDA
#define RDID2   0xDB
#define RDID3   0xDC
#define RDID4   0xDD

#define GMCTRP1 0xE0
#define GMCTRN1 0xE1

#define PWCTR6  0xFC


#define TFT_WIDTH  80
#define TFT_HEIGHT 160

// MV = 0 in MADCTL
// max columns
#define MAX_X 160
#define MIN_Y 26
// max rows
#define MAX_Y (MIN_Y + 80)
// columns max counter
#define SIZE_X MAX_X - 1
// rows max counter
#define SIZE_Y (MAX_Y - MIN_Y - 1)
// whole pixels
#define CACHE_SIZE_MEM (MAX_X * MAX_Y)
// number of columns for chars
#define CHARS_COLS_LEN 5
// number of rows for chars
#define CHARS_ROWS_LEN 8

/** @const Characters */
extern const uint8_t CHARACTERS[][CHARS_COLS_LEN];

/** @enum Font sizes */
typedef enum {
// normal font size: 1x high & 1x wide
X1 = 0x00,
// bigger font size: 2x higher & 1x wide
X2 = 0x01,
// the biggest font size: font 2x higher & 2x wider
// 0x0A is set cause offset 5 position to right only for
//      this case and no offset for previous cases X1, X2
//      when draw the characters of string in DrawString()
X3 = 0x0A
} ESizes;

