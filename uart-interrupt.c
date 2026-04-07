/*
*
*   uart-interrupt.c
*
*
*
*   @author
*   @date
*/

#include <inc/tm4c123gh6pm.h>    // Header file for TM4C123 microcontroller register definitions
#include <stdint.h>               // Standard integer types library (uint16_t, etc.)
#include "uart-interrupt.h"       // Custom header file for UART interrupt functions

// Global variables for UART communication (volatile because they're modified in interrupt context)
volatile char byte_received = '\0';    // Stores the most recently received byte
volatile char command_byte = -1;        // Command byte to compare received data against
volatile int command_flag = 0;          // Flag set when received byte matches command_byte

/**
 * Initialize UART1 module with interrupt capability
 * Configures GPIO, UART parameters, and interrupt handling
 */
void uart_interrupt_init(void){

    // Enable clock for GPIO Port B (0x02 = 0b0010, bit 1 for Port B)
    SYSCTL_RCGCGPIO_R |= 0x02;

    // Enable clock for UART1 module (0x02 = 0b0010, bit 1 for UART1)
    SYSCTL_RCGCUART_R |= 0x02;

    // Wait for GPIO Port B clock to be ready
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};

    // Wait for UART1 clock to be ready
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    // Configure GPIO Port B pins for UART functionality
    GPIO_PORTB_DEN_R   |= 0x03;        // Enable digital function on PB0 and PB1 (bits 0 and 1)
    GPIO_PORTB_AFSEL_R |= 0x03;        // Enable alternate function on PB0 and PB1 for UART
    GPIO_PORTB_PCTL_R   = (GPIO_PORTB_PCTL_R & 0xFFFFFF00) | 0x00000011;  // Set PCTL for PB0 (U1Rx) and PB1 (U1Tx)

    // UART baud rate configuration for 115200 baud (assuming 16MHz clock)
    uint16_t iBRD = 8;                  // Integer part of baud rate divisor
    uint16_t fBRD = 44;                 // Fractional part of baud rate divisor

    // Disable UART1 before configuration
    UART1_CTL_R  &= ~0x01;              // Clear bit 0 (UARTEN) to disable UART

    // Set baud rate divisor values
    UART1_IBRD_R  = iBRD;                // Integer baud rate divisor
    UART1_FBRD_R  = fBRD;                // Fractional baud rate divisor

    // Configure UART line control
    UART1_LCRH_R  = 0x60;                // 0x60 = 0b01100000: 8-bit data, FIFO enabled
    UART1_CC_R    = 0x0;                  // Use system clock as UART clock source

    // Configure UART interrupts
    UART1_ICR_R |= 0b00010000;            // Clear any pending receive interrupt (bit 4)
    UART1_IM_R  |= 0x10;                   // Enable receive interrupt (bit 4, RXIM)

    // Configure interrupt priority in NVIC (Nested Vectored Interrupt Controller)
    NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF0FFFFF) | 0x00200000;  // Set priority level for UART1

    // Enable UART1 interrupt in NVIC
    NVIC_EN0_R |= (1 << 6);                // Enable interrupt 6 (UART1) in NVIC

    // Register interrupt handler - tells CPU where to jump on UART1 interrupt
    IntRegister(INT_UART1, UART1_Handler);

    // Enable global interrupts at CPU level
    IntMasterEnable();

    // Enable UART1 module with interrupts
    UART1_CTL_R = 0x301;                   // Enable UART, TX, RX, and interrupts (bits 8,0)
}

/**
 * Send a single character via UART
 * @param data: Character to transmit
 */
void uart_sendChar(char data){
    // Wait until transmit FIFO is not full (bit 5 TXFF in UART_FR_R)
    while ((UART1_FR_R & 0x20) != 0) {}

    // Write data to data register for transmission
    UART1_DR_R = data;
}

/**
 * Receive a single character via UART (blocking)
 * @return Received character
 */
char uart_receive(void){
    // Wait until receive FIFO is not empty (bit 4 RXFE in UART_FR_R)
    while ((UART1_FR_R & 0x10) != 0) {}

    // Read and return received data (mask with 0xFF to get only lower 8 bits)
    return (char)(UART1_DR_R & 0xFF);
}

/**
 * Send a string via UART
 * @param data: Pointer to null-terminated string
 */
void uart_sendStr(const char *data){
    // Loop until null terminator is reached
    while (*data != '\0') {
        uart_sendChar(*data);    // Send current character
        data++;                    // Move to next character
    }
}

/**
 * UART1 Interrupt Service Routine (ISR)
 * Called automatically when UART1 receives data
 */
void UART1_Handler(void)
{
    // Check if receive interrupt occurred (bit 4, RXMIS in UART_MIS_R)
    if (UART1_MIS_R & 0x10)
    {
        // Clear the interrupt by writing to the Interrupt Clear Register
        UART1_ICR_R |= 0b00010000;

        // Read the received byte (mask with 0xFF to get only data bits)
        byte_received = (char)(UART1_DR_R & 0xFF);

        // Echo the received character back to sender
        uart_sendChar(byte_received);

        // Handle line endings - if carriage return received, add line feed
        if (byte_received == '\r')
        {
            uart_sendChar('\n');
        }
        else
        {
            // Check if received byte matches the command byte
            if (byte_received == command_byte)
            {
                command_flag = 1;    // Set flag indicating command match
            }
        }
    }
}
