/*
 * CybotGUIScan.c
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

#include "uart-interrupt.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "Timer.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>

//=============================================================================
// CALIBRATION - USE YOUR VALUES
//=============================================================================
#define MY_RIGHT_CAL     500     // Your right calibration value
#define MY_LEFT_CAL      2100    // Your left calibration value

extern int right_calibration_value;
extern int left_calibration_value;

// Global byte received from UART interrupt
extern volatile char byte_received;

//=============================================================================
// SCAN CONFIGURATION
//=============================================================================
#define SCAN_START       0
#define SCAN_END         180
#define SCAN_STEP        4        // 4 degree increments
#define NUM_SAMPLES      3        // Number of readings per angle
#define SAMPLE_DELAY_MS  5        // Delay between samples

//=============================================================================
// Helper: Average multiple ping readings at one angle
//=============================================================================
float ping_average(int angle, cyBOT_Scan_t *scan) {
    float sum = 0.0f;
    int valid = 0;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        cyBOT_Scan(angle, scan);

        // Valid range: 2cm to 300cm
        if (scan->sound_dist > 2.0f && scan->sound_dist < 300.0f) {
            sum += scan->sound_dist;
            valid++;
        }
        timer_waitMillis(SAMPLE_DELAY_MS);
    }

    if (valid > 0) {
        return sum / valid;  // Return average in cm
    }
    return -1.0f;  // Invalid reading
}

//=============================================================================
// Perform the scan and send data back to Python
//=============================================================================
void do_scan(void) {
    cyBOT_Scan_t scan;
    char buf[50];
    int angle;
    float distance_cm;
    float distance_m;

    lcd_printf("Scanning...");

    // Send start marker
    uart_sendStr("SCAN_START\n");

    // Send header (matches the format Python expects)
    uart_sendStr("Angle(Degrees)\tDistance(m)\n");

    // Perform the scan
    for (angle = SCAN_START; angle <= SCAN_END; angle += SCAN_STEP) {
        distance_cm = ping_average(angle, &scan);

        // Convert cm to meters for consistency
        if (distance_cm > 0) {
            distance_m = distance_cm / 100.0f;
        } else {
            // If invalid, use max distance (no obstacle)
            distance_m = 2.5f;
        }

        // Send data in the format Python expects
        sprintf(buf, "%d\t\t%.2f\n", angle, distance_m);
        uart_sendStr(buf);

        // Small delay to prevent buffer overflow
        timer_waitMillis(5);
    }

    // Send end marker
    uart_sendStr("END\n");
    uart_sendStr("SCAN_END\n");

    lcd_printf("Scan done!");
}

//=============================================================================
// MAIN
//=============================================================================
int main(void) {
    oi_t *sensor = oi_alloc();
    oi_init(sensor);
    uart_interrupt_init();
    lcd_init();
    timer_init();

    // Initialize servo and ping sensor
    cyBOT_init_Scan(0b0011);

    // Set calibration values
    right_calibration_value = MY_RIGHT_CAL;
    left_calibration_value = MY_LEFT_CAL;

    // Send startup message
    uart_sendStr("\nCyBot Ready - Press 'g' to scan\n");
    lcd_printf("Ready\nPress g");

    // Main loop - wait for 'g' command
    while (1) {
        // Check if 'g' was received
        if (byte_received == 'g') {
            byte_received = 0;  // Clear the flag

            uart_sendStr("\n>>> Starting Scan <<<\n");
            do_scan();
            uart_sendStr("\n>>> Scan Complete <<<\n");
            uart_sendStr("\nPress 'g' to scan again\n");

            lcd_printf("Scan done!\nPress g");
        }

        timer_waitMillis(10);
    }

    oi_free(sensor);
    return 0;
}
