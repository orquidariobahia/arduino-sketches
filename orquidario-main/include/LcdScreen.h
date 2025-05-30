#ifndef LCDSCREEN_H_
#define LCDSCREEN_H_

#define COLUMS           16   //LCD columns
#define ROWS             2    //LCD rows
#define LCD_SPACE_SYMBOL 0x20 //space symbol from LCD ROM, see p.9 of GDM2004D datasheet
// #define LCD_SCROLL_SPEED 350  //scroll speed in milliseconds
#define UPDATE_SCREEN 10000
#define DEGREE      0xDF

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

void updateScreen();

#endif