/*
 * LS013B7DH03.h
 *
 *  Created on: 28 Mar 2022
 *      Author: Gabriele
 */

#ifndef INC_LS013B7DH03_H_
#define INC_LS013B7DH03_H_

#include <stdint.h>
#include "shm_fonts.h"


#define LCD_HEIGHT			128
#define LCD_WIDTH          128

#define SHARPMEM_BIT_WRITECMD (0x01)     // 0x80 in LSB format
#define SHARPMEM_BIT_VCOM 	  (0x02)     // 0x40 in LSB format
#define SHARPMEM_BIT_CLEAR    (0x04)    // 0x20 in LSB format

// Enumeration for screen colors
typedef enum {
    Black = 0x00, // Black color, no pixel
    White = 0x01  // Pixel is set. Color depends on OLED
} LCD_COLOR;

// Struct to store display info
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} LCD_128x128_t;

void lcd_init(void);
void lcd_clear(void);
void test_lcd(void);
void lcd_refresh(void);
void lcd_DrawPixel(uint8_t x, uint8_t y, LCD_COLOR color);
char lcd_WriteChar(char ch, FontDef Font, LCD_COLOR color);

#endif /* INC_LS013B7DH03_H_ */
