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

// Libraries ------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "i2c0.h"
#include "gpio.h"
#include "timer.h"
#include "wait.h"
#include "adc0.h"
#include "plant.h"

#ifdef PLANT_DEBUG
#include "uart0.h"
#include "led_builtin.h"
#endif

// Defines --------------------------------------------------------------------

// BH1750 Ambient Light Sensor
#define BH_ADDRESS_VCC  0x5C    // Address when ADDR connected to VCC
#define BH_ADDRESS_GND  0x23    // Address when ADDR connected to GND
#define BH_POWER_ON     0x01
#define BH_RESET        0x07
#define BH_CONTINUOUS_H_RES_MODE_1  0x10 // 1lx res
#define BH_CONTINUOUS_H_RES_MODE_2  0x11 // 0.5lx res
#define BH_CONTINUOUS_L_RES_MODE    0x13 // 4lx res
#define BH_ONE_TIME_H_RES_MODE_1    0x20 // 1lx res
#define BH_ONE_TIME_H_RES_MODE_2    0x21 // 0.5lx res
#define BH_ONE_TIME_H_RES_MODE      0x23 // 4lx res
#define BH_H_MEASUREMENT_DELAY_US  120000  // 120ms delay for high res modes
#define BH_L_MEASUREMENT_DELAY_US  16000   // 16ms delay for low res modes

// DHT22 Temperature and Humidity Sensor
#define DH_OUT_PIN PORTD, 2     // Data output pin
#define DH_WAIT_TIME_SECONDS 2      // Time in seconds to wait for sensor to be ready

// Capacitive Soil Moisture Sensor
#define SM_AOUT_PIN PORTE, 0    // Analog output pin

// Water Pump Motor
#define WP_PWM_PIN PORTF, 2     // PWM pin
#define WP_GPO_PIN PORTF, 3     // Regular GPIO pin

// HX711 Weight Sensor
#define HX_DOUT_PIN PORTE, 1    // Data output pin
#define HX_SCK_PIN PORTE, 2     // PD Clock pin

// Variables ------------------------------------------------------------------

// DHT22 Temperature and Humidity Sensor
bool DHT22ready = true;     // Flag for sensor readiness

// Capacitive Soil Moisture Sensor
const float moistureMin = 1280.0; // Minimum value from the ADC (Water)
const float moistureMax = 2970.0; // Maximum value from the ADC (Air)

// Structures -----------------------------------------------------------------

// DHT22
typedef struct _dht22Data
{
  uint16_t hum;
  uint16_t temp;
  uint8_t checksum;
} dht22Data;

// Functions ------------------------------------------------------------------

// BH1750 Ambient Light Sensor
// TODO: Use 120ms timer for BH1750
// TODO: Add i2c error checking
// FIXME: Move BH1750 to I2C1 as it conflicts with ether

// Initializes the BH1750 Ambient Light sensor
void initBH1750(void)
{
    initI2c0();

    // BH1750 Startup
    writeI2c0Data(BH_ADDRESS_GND, BH_POWER_ON);
    writeI2c0Data(BH_ADDRESS_GND, BH_RESET);
    // Sets mode to 120ms 1lx resolution
    writeI2c0Data(BH_ADDRESS_GND, BH_CONTINUOUS_H_RES_MODE_1);

    // Waits 120ms between readings for high res mode
    waitMicrosecond(BH_H_MEASUREMENT_DELAY_US);
}

// Gets the lux value from the BH1750 sensor
uint16_t getBH1750Lux(void)
{
    // Variable for storing the sensor data
    uint8_t data[2];

    // Gets sensor data and stores it
    readI2c0Registers(BH_ADDRESS_GND, BH_CONTINUOUS_H_RES_MODE_1, data, 2);

    // Calculates lux value from sensor data
    uint16_t lux = ((data[0] << 8) | data[1]) / 1.2;

    // Waits 120ms between readings for high res mode
    waitMicrosecond(BH_H_MEASUREMENT_DELAY_US);

    return lux;
}

// DHT22 Temperature and Humidity Sensor
// TODO: Further test DHT22

// Initalizes the DHT22 Temperature and Humidity sensor
void initDHT22(void)
{
    enablePort(PORTD);

    // Sets the DHT22 pin as an output for initial communication
    selectPinPushPullOutput(DH_OUT_PIN);
    setPinValue(DH_OUT_PIN, 1);
}

// Callback function for the DHT22 sensor that sets the flag to true
void callbackDHT22(void)
{
    DHT22ready = true;
    KillTimer(callbackDHT22);
}

// Reads and stores the 40 bits of data from the DHT22 sensor
bool readDHT22Data(dht22Data *data)
{
    // Blocking function that waits until sensor is ready
    while (!DHT22ready)
    {
    }

    bool ok = false;
    volatile uint8_t mask = 0;
    volatile uint8_t count = 0;

    // Variables used for debugging, will get removed on final version
    volatile bool data_bits[40];
    uint8_t i = 0;
    for (i = 0; i < 40; i++)
    {
        data_bits[i] = 0;
    }
    volatile uint8_t counter_high_seg[40];
    volatile uint8_t counter_low_seg[40];
    volatile uint8_t counter_high = 0;
    volatile uint8_t counter_low = 0;

    // Sends start signal to the sensor
    setPinValue(DH_OUT_PIN, 0);
    waitMicrosecond(1000);
    setPinValue(DH_OUT_PIN, 1);
    waitMicrosecond(25);

    // Receive ready signal from sensor
    selectPinDigitalInput(DH_OUT_PIN); // Input mode to receive data
    waitMicrosecond(35);
    bool ready_low = getPinValue(DH_OUT_PIN); // Expecting low value
    waitMicrosecond(70);
    bool ready_high = getPinValue(DH_OUT_PIN); // Expecting high value
    waitMicrosecond(45);

    // Checks if ready signals were correct
    if (!ready_low && ready_high)
    {
        ok = true;
    }

    // Begin data transmission if ready signal was correct
    if (ok)
    {
        for (mask = 0; mask < 40; mask++)
        {
            // Blocking function while signal is low
            while (!getPinValue(DH_OUT_PIN))
            {
                counter_low++;
            }
            counter_low_seg[mask] = counter_low;
            counter_low = 0;

            // Counts how long the signal is high
            while (getPinValue(DH_OUT_PIN))
            {
                counter_high++;
            }
            // If the signal is high for more than 50 cycles, it's 1
            if (counter_high > 50)
            {
                // Humidity bitmask
                if (mask < 16)
                {
                    data->hum |= (1 << 15 - mask);
                }
                // Temperature bitmask
                else if (mask < 32)
                {
                    data->temp |= (1 << 15 - (mask - 16));
                }
                // Checksum bitmask
                else if (mask < 40)
                {
                    data->checksum |= (1 << 7 - (mask - 32));
                }

                data_bits[mask] = 1;
            }
            // Reset count for next bit
            counter_high_seg[mask] = counter_high;
            counter_high = 0;
        }

        // Calculate and verify checksum
        // Broken up to make it easier to read
        // Checksum =
        // ((h & 0xFF) + (h & 0xFF00 >> 8) + (t & 0xFF) + (t & 0xFF00 >> 8)) & 0xFF
        uint16_t check = 0;
        check = (data->hum & 0xFF) + ((data->hum & 0xFF00) >> 8);
        check = check + (data->temp & 0xFF) + ((data->temp & 0xFF00) >> 8);
        check = check & 0xFF;

        // If incorrect, will return false
        if (check != data->checksum)
        {
            ok = false;
        }
    }

    // Outputs 1 as to wait for next data transmission
    selectPinPushPullOutput(DH_OUT_PIN);
    setPinValue(DH_OUT_PIN, 1);

    startOneshotTimer(callbackDHT22, DH_WAIT_TIME_SECONDS);
    DHT22ready = false;

    return ok;
}

// Gets the temperature in Celcius from the DHT22 sensor
float getDHT22Temp(void)
{
    // Struct that holds sensor data
    dht22Data data;
    data.checksum = 0;
    data.temp = 0;
    data.hum = 0;

    // Return value
    float temp = 0.0;

    // Returns temperature in Celcius, rounded to 1 decimal place
    if (readDHT22Data(&data))
    {
        temp = (float)data.temp / 10;
    }
    // Indicates an error
    else
    {
        temp = 999.9;
    }

    return temp;
}

// Gets the humidity in percentage from the DHT22 sensor
float getDHT22Hum(void)
{
    // Struct that holds sensor data
    dht22Data data;
    data.checksum = 0;
    data.temp = 0;
    data.hum = 0;

    // Return value
    float hum = 0.0;

    // Returns humidity in percentage, rounded to 1 decimal place
    if (readDHT22Data(&data))
    {
        hum = (float)data.hum / 10;
    }
    // Indicates an error
    else
    {
        hum = 999.9;
    }

    return hum;
}

// Capacitive Soil Moisture Sensor

// Initializes the Capacitive Soil Moisture sensor
void initSoilMoistureSensor(void)
{
    enablePort(PORTE);

    // Sets the Soil Moisture sensor pin as an input to AIN3
    selectPinAnalogInput(SM_AOUT_PIN);

    // Initializes analog to digital converter
    initAdc0Ss3();
    // Use AIN3 input with N=4 hardware sampling
    setAdc0Ss3Mux(3);
    setAdc0Ss3Log2AverageCount(2);
}

// Gets the soil moisture percentage from the Capacitive Soil Moisture sensor
float getSoilMoisture(void)
{
    // Stores raw value from the ADC
    float raw = (float)readAdc0Ss3();
    // Return value with calcualted percentage
    float moisture = 0.0;

    // Calculates percentage: (1 - (raw - min) / (max - min)) * 100
    moisture = (1 - (raw - moistureMin) / (moistureMax - moistureMin) ) * 100;
    
    return moisture;
}

// Water Pump Motor
// TODO: Make sure water pump actually pumps water correctly lol

// Initializes the water pump for PWM control
void initWaterPump(void)
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    _delay_cycles(3);
    enablePort(PORTF);

    // Configure IN1 (PWM) and IN2 (Direction) outputs
    selectPinPushPullOutput(WP_PWM_PIN);
    setPinAuxFunction(WP_PWM_PIN, GPIO_PCTL_PF2_M1PWM6);
    selectPinPushPullOutput(WP_GPO_PIN);

    // Configure PWM module 1 to drive water pump motor
    // PWM on M1PWM6 (PF2), M0PWM3a
    SYSCTL_SRPWM_R |= SYSCTL_SRPWM_R1;               // reset PWM1 module
    SYSCTL_SRPWM_R &= ~SYSCTL_SRPWM_R1;              // leave reset state
    _delay_cycles(3);                                // wait 3 clocks
    PWM1_3_CTL_R = 0;                                // turn-off PWM1 generator 3
    PWM1_3_GENA_R = PWM_1_GENA_ACTCMPAD_ONE | PWM_1_GENA_ACTLOAD_ZERO;
    PWM1_3_LOAD_R = 255;                             // set period to 40 MHz sys clock / 2 / 255 = 78.431 kHz
    PWM1_3_CMPA_R = 0;                               // PWM off (0=always low, 1023=always high)
    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;                 // turn-on PWM1 generator 3
    PWM1_ENABLE_R = PWM_ENABLE_PWM6EN;               // enable PWM output

    // Sets direction to always be the same
    setPinValue(WP_GPO_PIN, 0);
}

// Sets the water pump PWM duty cycle
void setWaterPumpPWM(uint8_t duty)
{
    PWM1_3_CMPA_R = duty;
}

// HX711 Weight Sensor
// TODO: Finish writing HX711 functions

// Initializes the HX711
void initHX711(void)
{
    enablePort(PORTE);

    selectPinPushPullOutput(HX_SCK_PIN);
    selectPinDigitalInput(HX_DOUT_PIN);
}

// Reads and returns the data from the HX711
uint32_t readHX711Data(void)
{
    uint16_t i = 0;
    uint32_t data = 0;

    // Reads 24 bits of data from the HX711
    for (i = 0; i < 24; i++)
    {
        setPinValue(HX_SCK_PIN, 1);
        _delay_cycles(10);
        setPinValue(HX_SCK_PIN, 0);
        
        data <<= 1;
        data |= getPinValue(HX_DOUT_PIN);

        _delay_cycles(10);
    }

    // 25th clock pulse to set default gain and channel
    setPinValue(HX_SCK_PIN, 1);
    setPinValue(HX_SCK_PIN, 0);

    return data;
}

// Calculates and returns the volume in milliliters from the HX711
uint16_t getHX711Volume(void)
{
    uint32_t raw = readHX711Data();
    uint16_t volume = raw / 1000;

    return volume;
}

// Plant

// Initializes the plant peripherals
void initPlant(void)
{
    initBH1750();
    initDHT22();
    initSoilMoistureSensor();
    initWaterPump();
    // initHX711();
}

