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

void initBH1750(void);
uint16_t getBH1750Lux(void);

#endif