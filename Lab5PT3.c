/*
 * Lab5PT3.c
 *
 *  Created on: Mar 2, 2026
 *      Author: alecbroe
 */

#include "button.h"
#include "timer.h"
#include "lcd.h"
#include "uart.h"
#include "cyBot_Scan.h"

#define MAX 20

int main(void) {
    button_init();
    timer_init();
    lcd_init();
    uart_init();

    uart_sendStr("Part 3 Ready - local echo OFF, CyBot echoes for you\r\n");
    lcd_printf("Part 3 Ready");

    char buf[MAX + 1];
    int count = 0;

    while (1) {
        char c = uart_receive();

        if (c == '\r') {
            /* ENTER: send newline to PuTTY, display buffer on LCD, reset */
            uart_sendChar('\r');
            uart_sendChar('\n');
            buf[count] = '\0';
            lcd_clear();
            lcd_printf("%s", buf);
            count = 0;

        } else if (count >= MAX) {
            /* Buffer full: echo, display, reset */
            uart_sendChar(c);
            buf[MAX - 1] = c;
            buf[MAX] = '\0';
            lcd_clear();
            lcd_printf("%s", buf);
            uart_sendStr("\r\n[Buffer full - displayed on LCD]\r\n");
            count = 0;

        } else {
            /* Normal char: echo back and store */
            uart_sendChar(c);
            buf[count] = c;
            count++;
            buf[count] = '\0';
            lcd_clear();
            lcd_printf("%s\n%d", buf, count);
        }
    }

    return 0;
}





