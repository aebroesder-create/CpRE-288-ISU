/*
 * L5UART.c
 *
 *  Created on: Mar 2, 2026
 *      Author: alecbroe
 *
 * UART1 on Port B: PB0 = U1RX, PB1 = U1TX
 * 115200 baud, 8 data bits, no parity, 1 stop bit, no FIFO
 * System clock: 16 MHz, clock divisor: 16
 *
 * Baud rate math:
 *   BRD  = 16000000 / (16 * 115200) = 8.6805
 *   IBRD = 8
 *   FBRD = round(0.6805 * 64) = 44
 */

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include "uart.h"

void uart_init(void) {
    //enable clock to GPIO port B
    SYSCTL_RCGCGPIO_R |= 0x02;

    //enable clock to UART1
    SYSCTL_RCGCUART_R |= 0x02;

    //wait for GPIOB and UART1 peripherals to be ready
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
    while ((SYSCTL_PRUART_R & 0x02) == 0) {};

    //enable alternate functions on port B pins
    GPIO_PORTB_AFSEL_R |= 0x03;

    //enable digital functionality on port B pins
    GPIO_PORTB_DEN_R |= 0x03;

    //enable UART1 Rx and Tx on port B pins
    GPIO_PORTB_PCTL_R &= 0xFFFFFF00;
    GPIO_PORTB_PCTL_R |= 0x00000011;

    //calculate baud rate
    uint16_t iBRD = 8; //use equations
    uint16_t fBRD = 44; //use equations

    //turn off UART1 while setting it up
    UART1_CTL_R &= ~0x01;

    //set baud rate
    //note: to take effect, there must be a write to LCRH after these assignments
    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    //set frame, 8 data bits, 1 stop bit, no parity, no FIFO
    //note: this write to LCRH must be after the BRD assignments
    UART1_LCRH_R = 0x60;

    //use system clock as source
    //note from the datasheet UARTCCC register description:
    //field is 0 (system clock) by default on reset
    //Good to be explicit in your code
    UART1_CC_R = 0x0;

    //re-enable UART1 and also enable RX, TX (three bits)
    //note from the datasheet UARTCTL register description:
    //RX and TX are enabled by default on reset
    //Good to be explicit in your code
    //Be careful to not clear RX and TX enable bits
    //(either preserve if already set or set them)
    UART1_CTL_R = 0x301;
}

void uart_sendChar(char data) {
    while ((UART1_FR_R & 0x20) != 0) {} // Wait while TX FIFO full (bit 5)
    UART1_DR_R = (uint32_t)data;
}

char uart_receive(void) {
    while ((UART1_FR_R & 0x10) != 0) {} // Wait while RX FIFO empty (bit 4)
    return (char)(UART1_DR_R & 0xFF);
}

void uart_sendStr(const char *data) {
    while (*data != '\0') {
        uart_sendChar(*data);
        data++;
    }
}
