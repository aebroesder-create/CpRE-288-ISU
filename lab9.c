/*
 * lab9.c
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "lcd.h"
#include "ping.h"

int main(void) {
    char buffer[64];
    uint32_t last_measurement = 0;

    // Initialize system
    lcd_init();
    ping_init();

    lcd_clear();
    lcd_printf("PING Sensor Ready");

    while(1) {
        // Measure every 500ms
        if((TIMER0->TAV - last_measurement) > 40000000) {  // ~500ms at 80MHz
            // Trigger the sensor and get measurement
            ping_trigger();

            // Get measurements
            uint32_t pulse_clocks = ping_getPulseWidthClocks();
            float pulse_ms = ping_getPulseWidthMs();
            float distance = ping_getDistance();

            // Display results on LCD
            lcd_clear();

            // Line 1: Pulse width in clocks
            sprintf(buffer, "Clocks: %lu", pulse_clocks);
            lcd_printf("%s", buffer);

            // Line 2: Pulse width in ms
            sprintf(buffer, "Time: %.2f ms", pulse_ms);
            lcd_printf("%s", buffer);

            // Line 3: Distance
            sprintf(buffer, "Dist: %.1f cm", distance);
            lcd_printf("%s", buffer);

            // Line 4: Overflow count
            sprintf(buffer, "Overflow: %lu", g_overflow_count);
            lcd_printf("%s", buffer);

            last_measurement = TIMER0->TAV;
        }
    }

    return 0;
}
