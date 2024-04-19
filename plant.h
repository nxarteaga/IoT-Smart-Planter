/*
 * Rolando Rosales IoT Smart Planter Peripheral Library
 * github.com/rolo-g
*/

#ifndef PLANT_H_
#define PLANT_H_

// Libraries ------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "i2c0.h"
#include "wait.h"
#include "plant.h"

// Defines --------------------------------------------------------------------

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

// BH1750
void initBH1750(void);
uint16_t getBH1750Lux(void);

// DHT22
void initDHT22(void);
bool readDHT22Data(void);
uint16_t getDHT22Temp(void);
uint16_t getDHT22Hum(void);

#endif