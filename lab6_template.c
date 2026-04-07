/**
 * lab6_template.c
 *
 * Template file for CprE 288 Lab 6
 *
 * @author Diane Rover, 2/15/2020
 *
 */

#include "Timer.h"
#include "lcd.h"
#include "uart.h"
#include <stdio.h>

int main(void) {
    timer_init();
    lcd_init();
    uart_init();

    uart_sendStr("========================================\r\n");
    uart_sendStr("  CprE 288 Lab 6 - Part 1\r\n");
    uart_sendStr("  Non-blocking UART receive\r\n");
    uart_sendStr("  Press 'g' to start scan\r\n");
    uart_sendStr("  Press 's' to stop scan\r\n");
    uart_sendStr("========================================\r\n\r\n");

    lcd_printf("Lab6 Part1\nReady");

    int scanning = 0;
    int angle = 0;

    while (1) {

        // Non-blocking receive - returns 0 immediately if no key pressed
        // KEY CONCEPT: because this does NOT block, the loop keeps running
        // and can check for 's' stop command even during a scan.
        // With blocking uart_receive(), the stop command could never be
        // detected while a scan is in progress.
        char c = uart_receive_nonblocking();

        // Echo received character back to PuTTY
        if (c != 0) {
            uart_sendChar(c);
            if (c == '\r') uart_sendChar('\n');
        }

        // Go command - start scan
        if (c == 'g' && !scanning) {
            scanning = 1;
            angle = 0;
            uart_sendStr("\r\n>> Scan started! Press 's' to stop.\r\n");
            uart_sendStr("Angle\r\n");
            lcd_printf("Scanning...");
        }

        // Stop command - only reachable because receive is non-blocking
        if (c == 's' && scanning) {
            scanning = 0;
            uart_sendStr("\r\n>> Scan stopped!\r\n");
            lcd_printf("Stopped!");
        }

        // Simulate one scan step per loop iteration
        if (scanning) {
            if (angle <= 180) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d degrees\r\n", angle);
                uart_sendStr(buf);

                lcd_printf("Angle:\n%d", angle);

                // Small delay to simulate scan time
                timer_waitMillis(100);

                angle += 2;

            } else {
                scanning = 0;
                uart_sendStr("\r\n>> Scan complete!\r\n");
                lcd_printf("Scan Done!");
            }
        }

    }
}
