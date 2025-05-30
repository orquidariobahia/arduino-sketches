/* @file HelloKeypad.pde
|| @version 1.0
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Demonstrates the simplest use of the matrix Keypad library.
|| #
*/
#include <Keypad.h>
#include <Arduino.h>

#define KEY_0 '0'
#define KEY_1 '1'
#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_5 '5'
#define KEY_6 '6'
#define KEY_7 '7'
#define KEY_8 '8'
#define KEY_9 '9'
#define KEY_F1 'A'
#define KEY_F2 'B'
#define KEY_UP 'C'
#define KEY_DOWN 'D'
#define KEY_LEFT 'E'
#define KEY_RIGHT 'F'
#define KEY_ENT 'G'
#define KEY_ESC 'H'
#define KEY_STAR '*'
#define KEY_HASH '#'

const byte ROWS = 5; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {KEY_F1, KEY_F2, KEY_HASH, KEY_STAR},
  {KEY_1, KEY_2, KEY_3, KEY_UP},
  {KEY_4, KEY_5, KEY_6, KEY_DOWN},
  {KEY_7, KEY_8, KEY_9, KEY_ESC},
  {KEY_LEFT, KEY_0, KEY_RIGHT, KEY_ENT}
};
byte rowPins[ROWS] = { 8,7,6,5,4 }; //connect to the row pinouts of the keypad
byte colPins[COLS] = { 0,1,2,3 }; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
    Serial.begin(115200);
}

void loop() {
    char key = keypad.getKey();

    if (key) {
        // Serial.println(key);
        switch (key)
        {
        case KEY_F1:
            Serial.println("F1");
            break;
        case KEY_F2:
            Serial.println("F2");
            break;
        case KEY_UP:
            Serial.println("UP");
            break;
        case KEY_DOWN:
            Serial.println("DOWN");
            break;
        case KEY_LEFT:
            Serial.println("LEFT");
            break;
        case KEY_RIGHT:
            Serial.println("RIGHT");
            break;
        case KEY_ENT:
            Serial.println("ENT");
            break;
        case KEY_ESC:
            Serial.println("ESC");
            break;
        default:
            Serial.println(key);
            break;
        }
    }
}