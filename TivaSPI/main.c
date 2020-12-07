/*
 * Tiva SPI Example
 *
 * Copyright (c) 2020 - Terence M. Darwen - tmdarwen.com
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/*
 * Necessary Hardware:
 * - TI EK-TM4C1294XL Evaluation Board
 * - Microchip MCP23S17
 * - Typical through-hole LEDs
 * - 480 ohm through-hole resistors
 * - Typical breadboard
 * - Some jumpers
 *
 * Necessary Software:
 * - Code Composer Studio v9.x.x or later
 * - TI TivaWare Peripheral Driver Library v2.1.4.178 or later
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "drivers/pinout.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"

#define TIVA_PROCESSOR_FREQUENCY   120000000  // Set the Tiva processor to 120 MHz

#define TIVA_SSI_BIT_RATE              10000  // Set the SSI to transfer at a 10K bit rate
#define TIVA_SSI_DATA_WIDTH                8  // Set the SSI to transfer at 8 bits per frame

#define MCP23S17_SLAVE_ADDRESS          0x40  // See fig 3-5 of the MCP23S17 datasheet
#define MCP23S17_WRITE                  0x00  // See fig 3-5 of the MCP23S17 datasheet
#define MCP23S17_READ                   0x01  // See fig 3-5 of the MCP23S17 datasheet

#define MCP23S17_IODIRA_REG             0x00  // See table 3-1 of the MCP23S17 datasheet
#define MCP23S17_IODIRB_REG             0x01  // See table 3-1 of the MCP23S17 datasheet
#define MCP23S17_GPIOA_REG              0x12  // See table 3-1 of the MCP23S17 datasheet
#define MCP23S17_GPIOB_REG              0x13  // See table 3-1 of the MCP23S17 datasheet

#define MCP23S17_OUTPUT_CFG             0x00  // See Register 3-1 in the MCP23S17 datasheet

#define MCP23S17_POST_CS_LOW_DELAY      1000  // The delay after setting CS low, see fig 1-5 of the MCP23S17 datasheet
#define MCP23S17_PRE_CS_HIGH_DELAY      5000  // The delay before setting CS high, see fig 1-5 of the MCP23S17 datasheet
#define MCP23S17_POST_CS_HIGH_DELAY     1000  // The delay after setting CS high, see fig 1-5 of the MCP23S17 datasheet

#define MCP23S17_LEDS_OFF               0x00  // Set all MCP23S17 GPIO pins high, see Register 3-10 of datasheet
#define MCP23S17_LEDS_ON                0xFF  // Set all MCP23S17 GPIO pins low, see Register 3-10 of datasheet

#define LED_TOGGLE_DELAY            10000000  // The SysCtlDelay delay between toggling the LEDs high/low

void SetupMCP23S17();
void WriteToMCP23S17(unsigned char reg, unsigned char data);

int main(void)
{
    // Setup the Tiva's clock
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), TIVA_PROCESSOR_FREQUENCY);

    // Setup the MCP23S17
    SetupMCP23S17();

    // Indefinitely toggle the state of the LEDs
    bool ledsOn = true;
    while(1)
    {
        WriteToMCP23S17(MCP23S17_GPIOA_REG, ledsOn ? MCP23S17_LEDS_ON : MCP23S17_LEDS_OFF);
        WriteToMCP23S17(MCP23S17_GPIOB_REG, ledsOn ? MCP23S17_LEDS_ON : MCP23S17_LEDS_OFF);
        SysCtlDelay(LED_TOGGLE_DELAY);
        ledsOn = ledsOn ? false : true;
    }
}

void SetupMCP23S17()
{
    // We're using the Tiva's SSI2 (Synchronous Signal Interface) which uses port D pins for the SPI
    // clock, frame select and data. See Tiva TM4C1294NCPDT Microcontroller datasheet table 26-5 for
    // details.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    // The Tiva datasheet table 26-5 shows PD1 is the data and PD3 is the clock for SSI2
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_1 | GPIO_PIN_3);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    GPIOPinConfigure(GPIO_PD3_SSI2CLK);

    // The Tiva datasheet table 26-5 shows PD2 is the frame select (aka "chip select", aka CS, aka SS) 
    // for SSI2.  Normally we'd assign this similar to PD1 and PD3 and allow the SSI module toggle this
    // pin as necessary *but* due to the communication specification of the MCP23S17 (see fig 1-5 of
    // the MCP23S17 datasheet) we need to keep the CS low for all 3 bytes transferred.  We have to
    // do this manually.  Therefore we configure PD2 to be general purpose output and manually set it 
    // high/low as necessary in the WriteToMCP23S17 function.
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_2);

    // Enable the SSI2 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2));

    // Cofnigure the SSI2 peripheral
    SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, TIVA_SSI_BIT_RATE, TIVA_SSI_DATA_WIDTH);

    // Enable SSI2. Not sure why both SysCtlPeripheralEnable and SSIEnable must be called, but they do.
    SSIEnable(SSI2_BASE);

    // Initialize the CS to high. The CS needs to transition from high to low when starting communication
    // with the MCP23S17 (see fig 1-5 of the MCP23S17 datasheet).
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
    SysCtlDelay(MCP23S17_POST_CS_HIGH_DELAY);

    // Set all pins of ports A and B to output
    WriteToMCP23S17(MCP23S17_IODIRA_REG, MCP23S17_OUTPUT_CFG);
    WriteToMCP23S17(MCP23S17_IODIRB_REG, MCP23S17_OUTPUT_CFG);
}

void WriteToMCP23S17(unsigned char reg, unsigned char data)
{
    // Set the CS pin low.  See 17 (see fig 1-5 of the MCP23S17 datasheet) we need to keep the CS low for
    // all 3 bytes transferred.
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);
    SysCtlDelay(MCP23S17_POST_CS_LOW_DELAY);

    // Specify that we're doing a write to the MCP23S17
    SSIDataPut(SSI2_BASE, MCP23S17_SLAVE_ADDRESS | MCP23S17_WRITE);

    // Write the register and data bytes
    SSIDataPut(SSI2_BASE, reg);
    SSIDataPut(SSI2_BASE, data);

    // Set the CS pin back high to indicate the transmission is done, however, we need to make sure it
    // doesn't happen until all three bytes have been written, this is why we have the delays.
    SysCtlDelay(MCP23S17_PRE_CS_HIGH_DELAY);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
    SysCtlDelay(MCP23S17_POST_CS_HIGH_DELAY);
}
