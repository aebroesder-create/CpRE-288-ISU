/*
 * Lab8.c
 *
 *  Created on: Mar 24, 2026
 *      Author: alecbroe
 */

#include <stdint.h>
#include "adc.h"
#include "lcd.h"
#include "timer.h"

// Change this to 1 for Part 1, 2 for Part 2
#define PART 2

int main(void) {
    uint16_t raw;
    float dist;

    lcd_init();
    adc_init();

    lcd_clear();

#if PART == 1
    // PART 1: Display raw quantized values only
    lcd_printf("Part 1: Raw ADC");
    lcd_printf("Values:");

    while (1) {
        raw = adc_read();

        lcd_printf("Raw: %u   ", raw);
        timer_waitMillis(200);
        lcd_clear();
        lcd_printf("Part 1: Raw ADC");
        lcd_printf("Values:");
    }

#elif PART == 2
    // PART 2: Display both ADC value and distance
    lcd_printf("Part 2: Distance");
    lcd_printf("ADC   | Dist(cm)");

    while (1) {
        // Use software averaging for stable readings (16 samples)
        raw = adc_read_avg(16);
        dist = adc_to_distance(raw);

        lcd_printf(" %4d  |   %.1f", raw, dist);
        timer_waitMillis(200);
        lcd_clear();
        lcd_printf("Part 2: Distance");
        lcd_printf("ADC   | Dist(cm)");
    }
#endif

    return 0;
}
