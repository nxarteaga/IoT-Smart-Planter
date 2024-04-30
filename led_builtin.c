/*
 * Rolando Rosales 1001850424 - LED Builtin Library for TM4C v1.1
 *
 * Comments:
 * This is a very simple library that allows one to very easily control
 * the builtin LEDs on the TM4C123GH6PM MCU
 * Requires Dr. Losh's GPIO Library from CSE 4342 (Embedded Systems II)
*/

// Libraries ------------------------------------------------------------------

#include "gpio.h"
#include "led_builtin.h"

// Defines --------------------------------------------------------------------

// LED Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

// Red
void enableRedLED()
{
    setPinValue(RED_LED, true); 
}

void disableRedLED()
{
    setPinValue(RED_LED, false);
}

void toggleRedLED()
{
    togglePinValue(RED_LED);
}

// Green
void enableGreenLED()
{
    setPinValue(GREEN_LED, true);
}

void disableGreenLED()
{
    setPinValue(GREEN_LED, false);
}

void toggleGreenLED()
{
    togglePinValue(GREEN_LED);
}

// Blue
void enableBlueLED()
{
    setPinValue(BLUE_LED, true);
}

void disableBlueLED()
{
    setPinValue(BLUE_LED, false);
}

void toggleBlueLED()
{
    togglePinValue(BLUE_LED);
}

// All (White)
void enableAllLEDs()
{
    enableRedLED();
    enableGreenLED();
    enableBlueLED();
}

void disableAllLEDs()
{
    disableRedLED();
    disableGreenLED();
    disableBlueLED();
}

void toggleAllLEDs()
{
    toggleRedLED();
    toggleGreenLED();
    toggleBlueLED();
}
