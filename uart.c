/*
*
*   uart.c
*
*
*
*   @author
*   @date
*/

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include "uart.h"

void uart_init(void){

    // Enable clock to GPIO port B (bit 1)
    SYSCTL_RCGCGPIO_R |= 0x02;

    // Enable clock to UART1 (bit 1)
    SYSCTL_RCGCUART_R |= 0x02;

    // Wait for GPIOB and UART1 peripherals to be ready
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    // Enable alternate functions on PB0 (RX) and PB1 (TX)
    GPIO_PORTB_AFSEL_R |= 0x03;

    // Enable digital functionality on PB0 and PB1
    GPIO_PORTB_DEN_R |= 0x03;

    // Enable UART1 Rx and Tx on port B pins
    // PCTL: set PMC0 and PMC1 to 1 (UART function)
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFFFF00) | 0x00000011;

    // Baud rate calculation for 115200 baud at 16 MHz system clock:
    // BRD = 16,000,000 / (16 * 115200) = 8.6805
    // iBRD = 8
    // fBRD = round(0.6805 * 64) = round(43.55) = 44
    uint16_t iBRD = 8;
    uint16_t fBRD = 44;

    // Turn off UART1 while setting it up (clear bit 0 = UARTEN)
    UART1_CTL_R &= ~0x01;

    // Set baud rate
    // Note: LCRH must be written after BRD registers to take effect
    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    // Set frame: 8 data bits (WLEN=0x3), 1 stop bit, no parity, no FIFO
    // 0x60 = 0b01100000: bits 6:5 = WLEN = 11 (8-bit word length)
    UART1_LCRH_R = 0x60;

    // Use system clock as source (default, but explicit is good practice)
    UART1_CC_R = 0x0;

    // Re-enable UART1 (bit 0 = UARTEN), TX (bit 8 = TXE), RX (bit 9 = RXE)
    UART1_CTL_R = 0x301;
}

void uart_sendChar(char data){
    // Wait while TX FIFO is full (TXFF = bit 5 of Flag Register)
    while ((UART1_FR_R & 0x20) != 0) {}
    UART1_DR_R = data;
}

char uart_receive(void){
    // Wait while RX FIFO is empty (RXFE = bit 4 of Flag Register)
    while ((UART1_FR_R & 0x10) != 0) {}
    // Mask off upper error bits, return only the 8-bit data
    return (char)(UART1_DR_R & 0xFF);
}

// Lab 6 Part 1: Non-blocking receive
// Returns the received character if one is available, or 0 if not
char uart_receive_nonblocking(void){
    // If RX FIFO is empty (RXFE bit set), return 0 immediately
    if (UART1_FR_R & 0x10) {
        return 0;
    }
    return (char)(UART1_DR_R & 0xFF);
}

void uart_sendStr(const char *data){
    // Send each character one at a time until null terminator
    while (*data != '\0') {
        uart_sendChar(*data);
        data++;
    }
}














