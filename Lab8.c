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

// Set to 1 for Part 1 (raw ADC only), 2 for Part 2 (ADC + distance)
#define PART 2

int main(void) {
    lcd_init();
    adc_init();

    while (1) {
        uint16_t raw = adc_read();

#if PART == 1
        lcd_clear();
        lcd_printf("ADC Raw:\n%u", raw);

#elif PART == 2
        float dist = adc_to_distance(raw);
        lcd_clear();
        lcd_printf("ADC: %u\n", raw);
        lcd_printf("Dist: %.1f cm", dist);
#endif

        timer_waitMillis(200);
    }

    return 0;
}
