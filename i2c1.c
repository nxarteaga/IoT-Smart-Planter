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

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "i2c1.h"

// PortB masks
#define SDA_MASK 128
#define SCL_MASK 64

// Pins
#define I2C1SCL PORTA,6
#define I2C1SDA PORTA,7

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initI2c1(void)
{
    // Enable clocks
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R1;
    _delay_cycles(3);
    enablePort(PORTA);

    // Configure I2C
    selectPinPushPullOutput(I2C1SCL);
    setPinAuxFunction(I2C1SCL, GPIO_PCTL_PA6_I2C1SCL);
    selectPinOpenDrainOutput(I2C1SDA);
    setPinAuxFunction(I2C1SDA, GPIO_PCTL_PA7_I2C1SDA);

    // Configure I2C1 peripheral
    I2C1_MCR_R = 0;                                     // disable to program
    I2C1_MTPR_R = 19;                                   // (40MHz/2) / (6+4) / (19+1) = 100kbps
    I2C1_MCR_R = I2C_MCR_MFE;                           // master
    I2C1_MCS_R = I2C_MCS_STOP;
}

// For simple devices with a single internal register
void writeI2c1Data(uint8_t add, uint8_t data)
{
    I2C1_MSA_R = add << 1 | 0; // add:r/~w=0
    I2C1_MDR_R = data;
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
}

uint8_t readI2c1Data(uint8_t add)
{
    I2C1_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C1_MDR_R;
}

// For devices with multiple registers
void writeI2c1Register(uint8_t add, uint8_t reg, uint8_t data)
{
    // send address and register
    I2C1_MSA_R = add << 1; // add:r/~w=0
    I2C1_MDR_R = reg;
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);

    // write data to register
    I2C1_MDR_R = data;
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
    while (!(I2C1_MRIS_R & I2C_MRIS_RIS));
}

void writeI2c1Registers(uint8_t add, uint8_t reg, const uint8_t data[], uint8_t size)
{
    uint8_t i;
    // send address and register
    I2C1_MSA_R = add << 1; // add:r/~w=0
    I2C1_MDR_R = reg;
    if (size == 0)
    {
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
    }
    else
    {
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
        // first size-1 bytes
        for (i = 0; i < size-1; i++)
        {
            I2C1_MDR_R = data[i];
            I2C1_MICR_R = I2C_MICR_IC;
            I2C1_MCS_R = I2C_MCS_RUN;
            while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
        }
        // last byte
        I2C1_MDR_R = data[size-1];
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
    }
}

uint8_t readI2c1Register(uint8_t add, uint8_t reg)
{
    // set internal register counter in device
    I2C1_MSA_R = add << 1 | 0; // add:r/~w=0
    I2C1_MDR_R = reg;
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);

    // read data from register
    I2C1_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C1_MDR_R;
}

void readI2c1Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size)
{
    uint8_t i = 0;
    // send address and register number
    I2C1_MSA_R = add << 1; // add:r/~w=0
    I2C1_MDR_R = reg;
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);

    if (size == 1)
    {
        // add and read one byte
        I2C1_MSA_R = (add << 1) | 1; // add:r/~w=1
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C1_MDR_R;
    }
    else if (size > 1)
    {
        // add and first byte of read with ack
        I2C1_MSA_R = (add << 1) | 1; // add:r/~w=1
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_ACK;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C1_MDR_R;
        // read size-2 bytes with ack
        while (i < size-1)
        {
            I2C1_MICR_R = I2C_MICR_IC;
            I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_ACK;
            while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
            data[i++] = I2C1_MDR_R;
        }
        // last byte of read with nack
        I2C1_MICR_R = I2C_MICR_IC;
        I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C1_MDR_R;
    }
}

bool pollI2c1Address(uint8_t add)
{
    I2C1_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C1_MICR_R = I2C_MICR_IC;
    I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C1_MRIS_R & I2C_MRIS_RIS) == 0);
    return !(I2C1_MCS_R & I2C_MCS_ERROR);
}

bool isI2c1Error(void)
{
    return (I2C1_MCS_R & I2C_MCS_ERROR);
}

