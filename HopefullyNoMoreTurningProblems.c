/*
 * HopefullyNoMoreTurningProblems.c
 *
 *  Created on: Mar 26, 2026
 *      Author: alecbroe
 */

/*
 * movement.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */

#include "timer.h"
#include "lcd.h"
#include "cyBot_Scan.h"
#include "uart-interrupt.h"
#include <stdio.h>
#include <string.h>
#include "movement.h"
#include "open_interface.h"

extern volatile char byte_received;
extern int right_calibration_value;
extern int left_calibration_value;

int main(void) {
    // Initialize hardware
    timer_init();
    lcd_init();
    uart_interrupt_init();

    timer_waitMillis(100);

    oi_t *sensor = oi_alloc();
    oi_init(sensor);

    // Initialize scan library
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 500;
    left_calibration_value = 2100;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("DIRECTION VERIFICATION TEST\r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("Press 'l' - Turn LEFT for 1000ms\r\n");
    uart_sendStr("Press 'r' - Turn RIGHT for 1000ms\r\n");
    uart_sendStr("Press 'f' - Drive forward 200mm\r\n");
    uart_sendStr("Press '0' - Stop\r\n");
    uart_sendStr("========================================\r\n");

    while (1) {
        if (byte_received == 'l') {
            byte_received = 0;
            uart_sendStr("\r\n>>> Testing LEFT turn 1000ms <<<\r\n");
            uart_sendStr("Robot should turn LEFT (counter-clockwise)\r\n");
            oi_setWheels(150, -150);  // Left forward, Right backward = LEFT turn
            timer_waitMillis(1000);
            oi_setWheels(0, 0);
            uart_sendStr("Left turn test complete\r\n");
        }
        else if (byte_received == 'r') {
            byte_received = 0;
            uart_sendStr("\r\n>>> Testing RIGHT turn 1000ms <<<\r\n");
            uart_sendStr("Robot should turn RIGHT (clockwise)\r\n");
            oi_setWheels(-150, 150);  // Left backward, Right forward = RIGHT turn
            timer_waitMillis(1000);
            oi_setWheels(0, 0);
            uart_sendStr("Right turn test complete\r\n");
        }
        else if (byte_received == 'f') {
            byte_received = 0;
            uart_sendStr("\r\n>>> Testing drive forward 200mm <<<\r\n");
            move_front(sensor, 200);
            uart_sendStr("Drive test complete\r\n");
        }
        else if (byte_received == '0') {
            byte_received = 0;
            oi_setWheels(0, 0);
            uart_sendStr("\r\n>>> STOP <<<\r\n");
        }

        timer_waitMillis(10);
    }

    oi_free(sensor);
    return 0;
}
