/*
 * Rolando Rosales 1001850424 - CSE 4352 "IoT Smart Planter" Peripheral Library
 * 
 * Hardware setup:
 * BH1750 Ambient Light Sensor:
 * - SCL -> PB2
 * - SDA -> PB3
 * - ADDR -> GND
 * - 4.7k Pull-up resistors on SDA and SCL
 * 
 * DHT22 Temperature and Humidity Sensor:
 * - out -> PD2
 * 
 * Capacitive Soil Moisture Sensor:
 * - AOUT -> PE0
 * 
 * Water Pump Motor:
 * - IN1 -> PF2
 * - IN2 -> PF3
 * - Motor +- on either MOTOR-A pins
 * 
 * HX711 Weight Sensor:
 * - DT -> PE1
 * - SCK -> PE2
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
uint8_t getDHT22Temp(void);
uint8_t getDHT22Hum(void);
void getDHT22TempAndHum(uint8_t *temp, uint8_t *hum);

// Capacitive Soil Moisture Sensor
uint16_t getSoilMoisture(void);
uint32_t getSoilMoistureRaw(void);

// Water pump
void setWaterPumpSpeed(uint16_t duty);

// HX711 Weight Sensor
uint16_t getHX711Volume(void);

// Plant
void initPlant(void);
void getPlantData(uint16_t *lux, uint8_t *temp, uint8_t *hum, uint16_t *moist, uint16_t *volume);

#endif
