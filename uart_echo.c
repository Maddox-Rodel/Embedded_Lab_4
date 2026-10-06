//*****************************************************************************
//
// uart_echo.c - Example for reading data from and writing data to the UART in
//               an interrupt driven fashion.
//
// Copyright (c) 2012-2016 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.3.156 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/pwm.h"
#include <string.h>

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


uint32_t charCount = 0;

char command[4];


// PWM variables
uint32_t pwmPeriod;
volatile bool pwmRunning = false;


//*****************************************************************************
//
// UART Send Functions
//
//*****************************************************************************

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
        ROM_UARTCharPut(UART0_BASE, *pui8Buffer++);
    }
}


void UART1Send(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
        ROM_UARTCharPut(UART1_BASE, *pui8Buffer++);
    }
}


//*****************************************************************************
//
// Peripheral Initialization Functions
//
//*****************************************************************************

void GPIO_init(void)
{
    // Enable GPIO Port F
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
    {
    }

    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2);

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, 0);
}


void UART0_init(void)
{
    // Enable UART0 and GPIO Port A
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))
    {
    }

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
    {
    }

    // UART0 pin mux
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE,
                        GPIO_PIN_0 | GPIO_PIN_1);

    // UART0 baud/format
    ROM_UARTConfigSetExpClk(UART0_BASE,
          ROM_SysCtlClockGet(),
          115200,
          (UART_CONFIG_WLEN_8 |
          UART_CONFIG_STOP_ONE |
          UART_CONFIG_PAR_NONE));

    // UART0 interrupt enable
    ROM_IntEnable(INT_UART0);

    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}


void UART1_init(void)
{
    // Enable UART1 and GPIO Port B
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UART1))
    {
    }

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))
    {
    }

    // UART1 pin mux
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);

    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // UART1 baud/format
    ROM_UARTConfigSetExpClk(UART1_BASE,
                            ROM_SysCtlClockGet(),
                            9600,
                            (UART_CONFIG_WLEN_8 |
                             UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    // UART1 interrupt enable
    ROM_IntEnable(INT_UART1);

    ROM_UARTIntEnable(UART1_BASE,
                      UART_INT_RX | UART_INT_RT);
}


void PWM_init(void)
{
    // Enable PWM Module 1
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_PWM1))
    {
    }

    // PWM clock = System Clock / 64
    //
    // 40 MHz / 64 = 625 kHz
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    // GREEN LED is PF3
    // PF3 alternate function is M1PWM7
    GPIOPinConfigure(GPIO_PF3_M1PWM7);

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);

    pwmPeriod = 625;

    // M1PWM7 is controlled by PWM Generator 3
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);

    // Set PWM period
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, pwmPeriod);

    // Start GREEN LED at 50% duty cycle
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, pwmPeriod / 2);

    // Enable PWM output 7
    PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);

    // Enable PWM Generator 3
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}


//*****************************************************************************
//
// Command Functions
//
//*****************************************************************************

void forward(void)
{
    UART1Send((uint8_t *)"Move Forward\r\n", sizeof("Move Forward\r\n")-1);

    UARTSend((uint8_t *)"Move Forward\r\n", sizeof("Move Forward\r\n")-1);
}


void backward(void)
{
    UART1Send((uint8_t *)"Move Backwards\r\n", sizeof("Move Backwards\r\n")-1);
    UARTSend((uint8_t *)"Move Backwards\r\n", sizeof("Move Backwards\r\n")-1); 
}

void left(void)
{
    UART1Send((uint8_t *)"Move Left\r\n", sizeof("Move Left\r\n")-1) 
    UARTSend((uint8_t *)"Move Left\r\n", sizeof("Move Left\r\n")-1);
}


void right(void)
{
    UART1Send((uint8_t *)"Move Right\r\n", sizeof("Move Right\r\n")-1);
    UARTSend((uint8_t *)"Move Right\r\n", sizeof("Move Right\r\n")-1);
}


void startPWM(void)
{
    pwmRunning = true;

    // Turn RED and BLUE off
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, 0);

    UART1Send((uint8_t *)"PWM Started\r\n", sizeof("PWM Started\r\n")-1)     
    UARTSend((uint8_t *)"PWM Started\r\n", sizeof("PWM Started\r\n")-1);
}


//*****************************************************************************
//
// Lookup Table
//
//*****************************************************************************

typedef struct
{
    char command[4];
    void(*function)(void);
}
commandEntry;


commandEntry commandTable[] =
{
    {"FWD" , forward},
    {"BWD" , backward},
    {"LFT" , left},
    {"RGT" , right},
    {"PWM" , startPWM}
};


//*****************************************************************************
//
// UART0 Interrupt Handler
//
//*****************************************************************************

void UARTIntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    ROM_UARTIntClear(UART0_BASE, ui32Status);

    char inputChar;

    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        inputChar = ROM_UARTCharGetNonBlocking(UART0_BASE);

        ROM_UARTCharPutNonBlocking(UART0_BASE, inputChar);
    }
}


//*****************************************************************************
//
// UART1 Interrupt Handler
//
//*****************************************************************************

void UART1IntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = ROM_UARTIntStatus(UART1_BASE, true);

    ROM_UARTIntClear(UART1_BASE, ui32Status);

    char inputChar;

    while(ROM_UARTCharsAvail(UART1_BASE))
    {
        inputChar = ROM_UARTCharGetNonBlocking(UART1_BASE);

        ROM_UARTCharPutNonBlocking(UART1_BASE, inputChar);

        ROM_UARTCharPutNonBlocking(UART0_BASE, inputChar);

        charCount++;


        switch(charCount % 3)
        {
            case 1:

                // First character - BLUE
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_2);

                command[0] = inputChar;

                break;


            case 2:

                // Second character - RED
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_1);

                command[1] = inputChar;

                break;


            case 0:

                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, 0);

                command[2] = inputChar;

                command[3] = '\0';


                int i;

                for(i = 0; i < 5; i++)
                {
                    if(strcmp(command,
                              commandTable[i].command) == 0)
                    {
                        commandTable[i].function();

                        break;
                    }
                }

                break;
        }
    }
}


//*****************************************************************************
//
// Main
//
//*****************************************************************************

int
main(void)
{
    ROM_FPUEnable();

    ROM_FPULazyStackingEnable();


    //-------------------------------------------------------------------------
    // System Clock
    //
    // Configure system clock for 40 MHz
    // 400 MHz PLL / 2 / 5 = 40 MHz
    //-------------------------------------------------------------------------

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);


    //-------------------------------------------------------------------------
    // Initialize Peripherals
    //-------------------------------------------------------------------------

    GPIO_init();

    UART0_init();

    UART1_init();

    PWM_init();


    // Enable processor interrupts after peripherals are initialized
    ROM_IntMasterEnable();


    //-------------------------------------------------------------------------
    // Prompts
    //-------------------------------------------------------------------------

    UARTSend((uint8_t *)
             "Please enter 3-letter commands from the Bluetooth Terminal:\r\n", 
             sizeof("Please enter 3-letter commands from the Bluetooth Terminal:\r\n") - 1);


    UART1Send((uint8_t *)
              "Please enter 3-letter commands from the Bluetooth Terminal:\r\n", 
              sizeof("Please enter 3-letter commands from the Bluetooth Terminal:\r\n") - 1);


    //-------------------------------------------------------------------------
    // PWM Variables
    //
    // Start at 50% duty cycle
    //-------------------------------------------------------------------------

    uint32_t duty = pwmPeriod / 2;

    bool increasing = true;


    //-------------------------------------------------------------------------
    // Main Loop
    //-------------------------------------------------------------------------

    while(1)
    {
        if(pwmRunning)
        {
            // Increase duty cycle from 50% toward 100%
            if(increasing)
            {
                duty += 5;

                if(duty >= pwmPeriod - 1)
                {
                    duty = pwmPeriod - 1;

                    increasing = false;
                }
            }

            // Decrease duty cycle toward 0%
            else
            {
                if(duty > 5)
                {
                    duty -= 5;
                }

                else
                {
                    duty = 1;

                    increasing = true;
                }
            }

            // Configure PWM with new pulse width
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, duty);

            // Delay so brightness transition is visible
            SysCtlDelay(ROM_SysCtlClockGet() / 300);
        }
    }
}