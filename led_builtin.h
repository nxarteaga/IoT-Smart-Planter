/*
 * Rolando Rosales 1001850424 - LED Builtin Library for TM4C v1.1
 *
 * Comments:
 * This is a very simple library that allows one to very easily
 * control the builtin LEDs on the TM4C123GH6PM MCU
 * Requires Dr. Losh's GPIO Library from CSE 4342 (Embedded Systems II)
*/

#ifndef LED_BUILTIN_H_
#define LED_BUILTIN_H_

// Libraries ------------------------------------------------------------------

#include "gpio.h"

// Defines --------------------------------------------------------------------

// LED Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

// Red
void enableRedLED();
void disableRedLED();
void toggleRedLED();

// Green
void enableGreenLED();
void disableGreenLED();
void toggleGreenLED();

// Blue
void enableBlueLED();
void disableBlueLED();
void toggleBlueLED();

// All (White)
void enableAllLEDs();
void disableAllLEDs();
void toggleAllLEDs();

#endif
