/*
 * Calibration_helper.c
 *
 *  Created on: Mar 24, 2026
 *      Author: alecbroe
 */

#include "tm4c123gh6pm.h"
#include "lcd.h"
#include "timer.h"
#include "adc.h"

// Calibration structure
typedef struct {
    uint16_t raw_value;
    float distance_cm;
} CalibrationPoint;

#define MAX_POINTS 15
CalibrationPoint points[MAX_POINTS];
int point_count = 0;

void display_calibration_instructions(void) {
    lcd_clear();
    lcd_printf("IR Calibration");
    lcd_setCursor(0, 1);
    lcd_printf("Place object at");
    lcd_setCursor(0, 2);
    lcd_printf("known distance");
    lcd_setCursor(0, 3);
    lcd_printf("Press SW1 to log");
}

void record_calibration_point(float distance_cm) {
    uint16_t raw = adc_read_averaged(32);  // Average 32 samples

    points[point_count].raw_value = raw;
    points[point_count].distance_cm = distance_cm;
    point_count++;

    lcd_clear();
    lcd_printf("Point %d recorded", point_count);
    lcd_setCursor(0, 1);
    lcd_printf("Dist: %.1f cm", distance_cm);
    lcd_setCursor(0, 2);
    lcd_printf("Raw: %d", raw);
    timer_waitMillis(2000);
}

void print_calibration_data(void) {
    lcd_clear();
    lcd_printf("Calibration Data");

    for(int i = 0; i < point_count; i++) {
        lcd_setCursor(0, i+1);
        lcd_printf("%.1f cm -> %d", points[i].distance_cm, points[i].raw_value);
        timer_waitMillis(1000);
    }

    // Also print to serial if available
    // You can copy these values to your adc.c file
}

int main(void) {
    lcd_init();
    timer_init();
    adc_init();

    // Calibration distances (in cm)
    float distances[] = {9, 12, 15, 18, 20, 22, 25, 27, 30, 32, 35, 38, 40, 45, 50};
    int num_distances = 15;

    for(int i = 0; i < num_distances && point_count < MAX_POINTS; i++) {
        display_calibration_instructions();

        lcd_setCursor(0, 3);
        lcd_printf("Current: %.1f cm", distances[i]);

        // Wait for button press (simplified - use actual button check)
        timer_waitMillis(3000);

        record_calibration_point(distances[i]);
    }

    print_calibration_data();

    while(1) {
        // Display live readings after calibration
        uint16_t raw = adc_read_averaged(16);
        lcd_setCursor(0, 0);
        lcd_printf("Live: %d      ", raw);
        timer_waitMillis(100);
    }
}

