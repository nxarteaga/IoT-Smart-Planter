/*
 * Rolando Rosales IoT Smart Planter Peripheral Library
 * github.com/rolo-g
*/

// Libraries ------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "i2c0.h"
#include "wait.h"
#include "plant.h"

// Defines --------------------------------------------------------------------

// BH1750
#define BH1750_ADDRESS_VCC  0x5C
#define BH1750_ADDRESS_GND  0x23
#define POWER_ON            0x01
#define RESET               0x07
#define CONTINUOUS_H_RES_MODE_1 0x10 // 120ms 1lx res
#define CONTINUOUS_H_RES_MODE_2 0x11 // 120ms 0.5lx res
#define CONTINUOUS_L_RES_MODE   0x13 // 16ms 4lx res
#define ONE_TIME_H_RES_MODE_1   0x20 // 120ms 1lx res
#define ONE_TIME_H_RES_MODE_2   0x21 // 120ms 0.5lx res
#define ONE_TIME_H_RES_MODE     0x23 // 16ms 4lx res
#define H_MEASUREMENT_DELAY 120000
#define L_MEASUREMENT_DELAY 16000

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

void initBH1750(void)
{
    initI2c0();

    // BH1750 Startup
    writeI2c0Data(BH1750_ADDRESS_GND, POWER_ON);
    writeI2c0Data(BH1750_ADDRESS_GND, RESET);
    // Sets mode to 120ms 1lx resolution
    writeI2c0Data(BH1750_ADDRESS_GND, CONTINUOUS_H_RES_MODE_1);

    waitMicrosecond(H_MEASUREMENT_DELAY);
}

uint16_t getBH1750Lux(void)
{
    // Variable for storing the sensor data
    uint8_t data[2];

    // Gets sensor data and stores it
    readI2c0Registers(BH1750_ADDRESS_GND, CONTINUOUS_H_RES_MODE_1, data, 2);

    // Calculates lux value from sensor data
    uint16_t lux = ((data[0] << 8) | data[1]) / 1.2;

    // Waits 120ms between readings for high res mode
    waitMicrosecond(H_MEASUREMENT_DELAY);

    return lux;
}
