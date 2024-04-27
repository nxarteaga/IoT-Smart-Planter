// Rolando Rosales (1001850424)
// I2C0 Library - Modified for I2C1
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// I2C devices on I2C bus 1 with 2kohm pullups on SDA and SCL

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef I2C1_H_
#define I2C1_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initI2c1(void);
// For simple devices with a single internal register
void writeI2c1Data(uint8_t add, uint8_t data);
uint8_t readI2c1Data(uint8_t add);

// For devices with multiple registers
void writeI2c1Register(uint8_t add, uint8_t reg, uint8_t data);
void writeI2c1Registers(uint8_t add, uint8_t reg, const uint8_t data[], uint8_t size);
uint8_t readI2c1Register(uint8_t add, uint8_t reg);
void readI2c1Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size);

// General functions
bool pollI2c1Address(uint8_t add);
bool isI2c1Error(void);

#endif
