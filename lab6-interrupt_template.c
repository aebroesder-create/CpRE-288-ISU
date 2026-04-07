/**
 * lab6-interrupt_template.c
 *
 * Template file for CprE 288 Lab 6
 *
 * @author Diane Rover, 2/15/2020
 *
 */

#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include <stdio.h>

int main(void) {

    // timer_init() must be first - lcd_init() depends on it
    timer_init();
    lcd_init();
    // replaces uart_init() - sets up UART1 with interrupt support
    uart_interrupt_init();

    uart_sendStr("========================================\r\n");
    uart_sendStr("  CprE 288 Lab 6 - Part 3 Bonus\r\n");
    uart_sendStr("  Interrupt go/stop commands\r\n");
    uart_sendStr("  Press 'g' to start scan\r\n");
    uart_sendStr("  Press 's' to stop scan via interrupt\r\n");
    uart_sendStr("========================================\r\n\r\n");

    lcd_printf("Lab6 Part3\nReady");

    // tell ISR to watch for 's' - when received, ISR sets command_flag=1
    command_byte = 's';

    int scanning = 0;
    int angle    = 0;

    while(1)
    {
        // byte_received is written by ISR - check it for 'g' go command
        if (byte_received != '\0') {
            lcd_printf("Received:\n%c", byte_received);

            // start scan when 'g' received and not already scanning
            if (byte_received == 'g' && !scanning) {
                scanning = 1;
                angle    = 0;
                uart_sendStr("\r\n>> Scan started! Press 's' to stop.\r\n");
                uart_sendStr("Angle\r\n");
                lcd_printf("Scanning...");
            }

            byte_received = '\0';  // reset after reading
        }

        // ISR set command_flag=1 when 's' was received
        // reset flag immediately so it doesn't keep firing
        if (command_flag == 1) {
            command_flag = 0;
            if (scanning) {
                scanning = 0;
                uart_sendStr("\r\n>> Scan STOPPED by interrupt!\r\n");
                lcd_printf("Stopped by\nInterrupt!");
            }
        }

        // one step per loop so command_flag is checked between every step
        // a for loop here would block main() and ignore the stop command
        if (scanning) {
            if (angle <= 180) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d degrees\r\n", angle);
                uart_sendStr(buf);
                lcd_printf("Angle:\n%d", angle);
                timer_waitMillis(100);  // simulate scan step time
                angle += 2;
            } else {
                // reached 180 naturally
                scanning = 0;
                uart_sendStr("\r\n>> Scan complete!\r\n");
                lcd_printf("Scan Done!");
            }
        }
    }
}
