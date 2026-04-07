/*
 * lab10.c
 *
 *  Created on: Apr 6, 2026
 *      Author: alecbroe
 */

#include "servo.h"
#include "Timer.h"

int main(void) {
    // Initialize standard timer and servo
    timer_init();
    servo_init();

    while (1) {
        // Move to 0 degrees
        servo_move(0);
        timer_waitMillis(2000);

        // Move to 90 degrees
        servo_move(90);
        timer_waitMillis(2000);

        // Move to 180 degrees
        servo_move(180);
        timer_waitMillis(2000);
    }
}
