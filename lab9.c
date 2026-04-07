/*
 * lab9.c
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/tm4c123gh6pm.h"
#include "ping.h"
#include "lcd.h"
#include "timer.h"

int main(void) {

    uint32_t pulse_clocks;
    char line1[17];
    char line2[17];

    lcd_init();
    ping_init();

    lcd_clear();
    lcd_printf("Lab9 PING Ready");
    timer_waitMillis(1000);

    while (1) {

        ping_trigger();
        pulse_clocks = ping_get_pulse_width_clocks();

        snprintf(line1, sizeof(line1), "CYC:%lu", (unsigned long)pulse_clocks);

        if (ping_overflow_occurred()) {
            snprintf(line2, sizeof(line2), "OVF cnt=%lu", (unsigned long)ping_get_overflow_count());
        } else {
            snprintf(line2, sizeof(line2), "No overflow");
        }

        lcd_clear();
        lcd_printf("%s\n%s", line1, line2);

        timer_waitMillis(300);
    }
}
