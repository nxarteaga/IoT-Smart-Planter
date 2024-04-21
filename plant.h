/*
 * Rolando Rosales IoT Smart Planter Peripheral Library
 * github.com/rolo-g
*/

#ifndef PLANT_H_
#define PLANT_H_

// Libraries ------------------------------------------------------------------

#include <stdint.h>
#include "plant.h"

// Defines --------------------------------------------------------------------

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

// BH1750
uint16_t getBH1750Lux(void);

// DHT22
float getDHT22Temp(void);
float getDHT22Hum(void);

// Capacitive Soil Moisture Sensor
float getSoilMoisture(void);

// Water pump
void setWaterPumpPWM(uint8_t duty);

// HX711 Weight Sensor
uint16_t getHX711Volume(void);

// Plant
void initPlant(void);

#endif
