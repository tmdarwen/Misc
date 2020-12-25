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
 * - Maxim MAX541 DAC
 * - A few capacitors and resistors (see schematic)
 * - TS audio jack
 * - Typical breadboard
 * - Some jumpers
 *
 * Necessary Software:
 * - Code Composer Studio v9.x.x or later
 * - TI TivaWare Peripheral Driver Library v2.1.4.178 or later
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "drivers/pinout.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "PianoSample.h"

#define TIVA_PROCESSOR_FREQUENCY   120000000  // Set the Tiva processor to 120 MHz

#define TIVA_SSI_BIT_RATE              30700  // Haven't dug into this value, but it seems to get us close to 44.1KHz
#define TIVA_SSI_DATA_WIDTH               16  // The MAX541 is a 16 bit DAC

void SetupBoardLED();
void SetBoardLED(bool ledOn);
void SetupSPI();

int main(void)
{
    // Setup the Tiva's clock
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), TIVA_PROCESSOR_FREQUENCY);

    // Do configuration for the board's LED
    SetupBoardLED();

    // Do SPI configuration
    SetupSPI();

    bool ledsOn = true;
    uint32_t sampleIndex = 0;

    // Send the PianoSample.h data to the DAC indefinitely
    while(1)
    {
        // Note that SSIDataPut is a blocking function.  It'll wait until there's space available to "put" the data.
        SSIDataPut(SSI2_BASE, audioSample[sampleIndex++]);
        if(sampleIndex == audioSampleLength)
        {
            // Reset the sample index
            sampleIndex = 0;

            // Toggle the LED
            SetBoardLED(ledsOn);
            ledsOn = ledsOn ? false : true;
        }
    }
}

void SetupBoardLED()
{
    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Check if the peripheral access is enabled.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
}

void SetBoardLED(bool ledOn)
{
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, ledOn ? GPIO_PIN_0 : 0);
}

void SetupSPI()
{
    // We're using the Tiva's SSI2 (Synchronous Signal Interface) which uses port D pins for the SPI
    // clock, frame select and data. See Tiva TM4C1294NCPDT Microcontroller datasheet table 26-5 for
    // details.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    // The Tiva datasheet table 26-5 shows PD1 is the data and PD3 is the clock for SSI2
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    GPIOPinConfigure(GPIO_PD2_SSI2FSS);
    GPIOPinConfigure(GPIO_PD3_SSI2CLK);

    // Enable the SSI2 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2));

    // Cofnigure the SSI2 peripheral
    SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, TIVA_SSI_BIT_RATE, TIVA_SSI_DATA_WIDTH);

    // Enable SSI2. Not sure why both SysCtlPeripheralEnable and SSIEnable must be called, but they do.
    SSIEnable(SSI2_BASE);
}
