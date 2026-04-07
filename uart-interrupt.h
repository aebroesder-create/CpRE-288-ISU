/*
*
*   uart-interrupt.h
*
*   Used to set up the RS232 connector and WIFI module
*   Uses RX interrupt
*   Functions for communicating between CyBot and PC via UART1
*   Serial parameters: Baud = 115200, 8 data bits, 1 stop bit,
*   no parity, no flow control on COM1, FIFOs disabled on UART1
*
*   @author Dane Larson
*   @date 07/18/2016
*   Phillip Jones updated 9/2019, removed WiFi.h, Timer.h
*   Diane Rover updated 2/2020, added interrupt code
*/

#ifndef UART_INTERRUPT_H_
#define UART_INTERRUPT_H_

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"

// Global variables defined in uart-interrupt.c
// extern makes them visible to any file that includes this header
extern volatile char byte_received; // creating this makes a seperate variable that is always check/ "read" and can be updated - different keys pressed
extern volatile char command_byte;
extern volatile int  command_flag;

// UART1 initialization with RX interrupt support
void uart_interrupt_init(void);

// Send one byte over UART1
void uart_sendChar(char data);

// Blocking receive - DO NOT USE when interrupts are active
char uart_receive(void);

// Send a string over UART1
void uart_sendStr(const char *data);

// ISR - never call this, CPU calls it automatically
void UART1_Handler(void);

#endif /* UART_INTERRUPT_H_ */

