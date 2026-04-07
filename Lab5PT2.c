/*
 * Lab5PT2.c
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

    uart_sendStr("Part 2 Ready - type up to 20 chars or press ENTER\r\n");
    lcd_printf("Part 2 Ready");

    char buf[MAX + 1];
    int count = 0;

    while (1) {
        char c = uart_receive();

        if (c == '\r' || count >= MAX) {
            /* Flush: show full buffer on LCD then reset */
            buf[count] = '\0';
            lcd_clear();
            lcd_printf("%s", buf);
            count = 0;
            uart_sendStr("\r\n[Displayed on LCD]\r\n");

        } else {
            /* Store char, update LCD with current string and count */
            buf[count] = c;
            count++;
            buf[count] = '\0';
            lcd_clear();
            lcd_printf("%s\n%d", buf, count);
        }
    }

    return 0;
}
