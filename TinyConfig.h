#ifndef TINYCONFIG_H
#define TINYCONFIG_H

#ifndef TINYSCREENSIM
#define TINYARCADE_CONFIG
//#define TINYSCREEN_GAMEKIT_CONFIG
#else
#define TINYSCREEN_GAMEKIT_CONFIG
#endif

#ifdef TINYARCADE_CONFIG
#define pgm_read_ptr(x) (*(x))

const uint8_t PROGMEM TAJoystick2Up  = 1 << 4;       //Mask
const uint8_t PROGMEM TAJoystick2Down = 1 << 5;      //Mask
const uint8_t PROGMEM TAJoystick2Left  = 1 << 6;     //Mask
const uint8_t PROGMEM TAJoystick2Right = 1 << 7;     //Mask

#define TINYSCREEN_TYPE TinyScreenPlus
#endif
#ifdef TINYSCREEN_GAMEKIT_CONFIG
#include <avr/pgmspace.h>

#define TINYSCREEN_TYPE 0
#define SerialUSB Serial
#endif

#endif
