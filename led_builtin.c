/*
 * Rolando Rosales LED Builtin Library for TM4C v1.0
 * github.com/rolo-g
 *
 * Comments:
 * This is a very simple library that allows one to very easily
 * control the builtin LEDs on the TM4C123GH6PM MCU
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