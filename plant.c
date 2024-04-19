/*
 * Rolando Rosales (1001850424) IoT Smart Planter Peripheral Library
 * github.com/rolo-g
*/

// Libraries ------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "i2c0.h"
#include "gpio.h"
#include "wait.h"
#include "plant.h"
#include "led_builtin.h"

// Defines --------------------------------------------------------------------

// Preprocessor directives
#define DH_DEBUG    // Prints debug messages for DHT22 sensor

// BH1750
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
#define BH_H_MEASUREMENT_DELAY  120000  // 120ms delay for high res modes
#define BH_L_MEASUREMENT_DELAY  16000   // 16ms delay for low res modes

// DHT22
#define DH_OUT_PIN PORTD, 2     // Data output pin

// Variables ------------------------------------------------------------------

// Structures -----------------------------------------------------------------

// Functions ------------------------------------------------------------------

// BH1750
// TODO: Use timer to wait between readings, instead of waitMicrosecond

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
    waitMicrosecond(BH_H_MEASUREMENT_DELAY);
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
    waitMicrosecond(BH_H_MEASUREMENT_DELAY);

    return lux;
}

// DHT22

// Initalizes the DHT22 Temperature and Humidity sensor
void initDHT22(void)
{
    // Enables Port D for the DHT22 sensor
    enablePort(PORTD);

    // Sets the DHT22 pin as an output for initial communication
    selectPinPushPullOutput(DH_OUT_PIN);
    setPinValue(DH_OUT_PIN, 1);
}

// Reads and stores the 40 bits of data from the DHT22 sensor
bool readDHT22Data(dht22Data *data)
{
    bool ok = false;
    volatile uint8_t mask = 0;
    volatile uint8_t counter = 0;


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
    waitMicrosecond(50);

    // Checks if ready signals were correct
    if (!ready_low && ready_high)
    {
        ok = true;
        enableGreenLED();
    }

    // Begin data transmission if ready signal was correct
    if (ok)
    {
        for (mask = 0; mask < 40; mask++)
        {
            while (!getPinValue(DH_OUT_PIN))
            {
                counter_low++;
            }
            counter_low_seg[mask] = counter_low;
            counter_low = 0;

            while (getPinValue(DH_OUT_PIN))
            {
                counter_high++;
            }
            if (counter_high > 50)
            {
                data->hum |= (mask << 1);
                data_bits[mask] = 1;
            }
            counter_high_seg[mask] = counter_high;
            counter_high = 0;
        }
    }

    // Outputs 1 as to wait for next data transmission
    selectPinPushPullOutput(DH_OUT_PIN);
    setPinValue(DH_OUT_PIN, 1);

    // checksum calc goes here

    return ok;
}

// Gets the temperature in Celcius from the DHT22 sensor
uint16_t getDHT22Temp(void)
{
    uint32_t temp;
    dht22Data data;

    if(readDHT22Data(&data))
    {
    }

    return temp;
}

// Gets the humidity in percentage from the DHT22 sensor
uint16_t getDHT22Hum(void)
{
    uint16_t hum = 0;
    uint16_t data[3];

    if (readDHT22Data(&data))
    {
    }

    return hum;
}
