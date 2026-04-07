/*
 * lab6PT2.c
 *
 *  Created on: Mar 9, 2026
 *      Author: alecbroe
 */

#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"

int main(void) {

    // timer_init() must be first - lcd_init() depends on it
    timer_init();
    lcd_init();
    // replaces uart_init() - sets up UART1 with interrupt support
    uart_interrupt_init();

    uart_sendStr("========================================\r\n");
    uart_sendStr("  CprE 288 Lab 6 - Part 2\r\n");
    uart_sendStr("  Interrupt-driven UART\r\n");
    uart_sendStr("  Type anything - ISR echoes it back\r\n");
    uart_sendStr("  Watch LCD update with each keypress\r\n");
    uart_sendStr("========================================\r\n\r\n");

    lcd_printf("Lab6 Part2\nReady");

    while(1)
    {
        // byte_received is updated by UART1_Handler automatically
        // '\0' means nothing new - ISR sets it when a char arrives
        // volatile ensures main() always reads the latest ISR value
        if (byte_received != '\0') {
            lcd_printf("Received:\n%c", byte_received);
            byte_received = '\0';  // reset after reading
        }
    }
}
