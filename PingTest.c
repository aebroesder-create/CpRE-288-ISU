/*
 * PingTest.c
 *
 *  Created on: Mar 23, 2026
 *      Author: alecbroe
 */

#include "cyBot_Scan.h"
#include "open_interface.h"
#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include <stdio.h>

// ── UPDATE with your calibration values ────────────────────────────────────
#define MY_RIGHT_CAL      500
#define MY_LEFT_CAL       2100

int main(void) {
    char buf[100];
    int angle;
    float dist;

    timer_init();
    oi_t *sensor = oi_alloc();
    oi_init(sensor);
    uart_interrupt_init();
    lcd_init();

    cyBOT_init_Scan(0b0111);
    right_calibration_value = MY_RIGHT_CAL;
    left_calibration_value  = MY_LEFT_CAL;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("PING SENSOR TEST\r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("\r\nThis test will scan from 0 to 180 degrees\r\n");
    uart_sendStr("and show PING readings at each angle.\r\n");
    uart_sendStr("\r\nPlace a solid object (book, box, your hand)\r\n");
    uart_sendStr("10-50cm in front of the robot.\r\n");
    uart_sendStr("\r\nPress 's' to start testing...\r\n");
    lcd_printf("Ping Test\nPress s");

    while (1) {
        if (byte_received == 's') {
            byte_received = '\0';
            cyBOT_Scan_t scan;

            uart_sendStr("\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("Angle   PING Distance (cm)\r\n");
            uart_sendStr("-----   -----------------\r\n");

            // Test every 10 degrees first to find where object is
            for (angle = 0; angle <= 180; angle += 10) {
                cyBOT_Scan(angle, &scan);
                sprintf(buf, "%3d     ", angle);
                uart_sendStr(buf);

                if (scan.sound_dist > 0 && scan.sound_dist < 300) {
                    sprintf(buf, "%.1f cm\r\n", scan.sound_dist);
                    uart_sendStr(buf);
                    lcd_printf("Angle %d\n%.1f cm", angle, scan.sound_dist);
                } else {
                    uart_sendStr("NO ECHO\r\n");
                    lcd_printf("Angle %d\nNO ECHO", angle);
                }
                timer_waitMillis(500);
            }

            uart_sendStr("\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("Now testing fine scan around detected area...\r\n");
            uart_sendStr("Place object at 90 degrees (straight ahead)\r\n");
            uart_sendStr("Press 's' when ready...\r\n");
            lcd_printf("Fine scan\nPress s");

            // Wait for second press
            while (byte_received != 's') {
                // Wait
            }
            byte_received = '\0';

            uart_sendStr("\r\n");
            uart_sendStr("Fine scan (80-100 degrees):\r\n");
            uart_sendStr("Angle   PING Distance (cm)\r\n");
            uart_sendStr("-----   -----------------\r\n");

            // Fine scan around 90 degrees
            for (angle = 80; angle <= 100; angle += 2) {
                cyBOT_Scan(angle, &scan);
                sprintf(buf, "%3d     ", angle);
                uart_sendStr(buf);

                if (scan.sound_dist > 0 && scan.sound_dist < 300) {
                    sprintf(buf, "%.1f cm\r\n", scan.sound_dist);
                    uart_sendStr(buf);
                    lcd_printf("Angle %d\n%.1f cm", angle, scan.sound_dist);
                } else {
                    uart_sendStr("NO ECHO\r\n");
                    lcd_printf("Angle %d\nNO ECHO", angle);
                }
                timer_waitMillis(300);
            }

            uart_sendStr("\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("PING TEST COMPLETE\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("\r\nIf you see 'NO ECHO' at all angles:\r\n");
            uart_sendStr("1. Check PING sensor connection\r\n");
            uart_sendStr("2. Make sure object is 10-50cm away\r\n");
            uart_sendStr("3. Use a flat, hard object (book, cardboard)\r\n");
            uart_sendStr("\r\nPress 's' to test again.\r\n");
            lcd_printf("Test Done\nPress s");
        }
    }

    oi_free(sensor);
    return 0;
}
