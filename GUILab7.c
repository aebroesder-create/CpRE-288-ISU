/*
 * GUILab7.c
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

/*
 * GUILab7.c
 * Complete working version for Python GUI
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

/*
 * GUILab7.c
 * Complete working version for Python GUI with Stop support
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

#include "uart-interrupt.h"
#include "cyBot_Scan.h"
#include "Timer.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>

//=============================================================================
// CALIBRATION VALUES - USE YOUR VALUES
//=============================================================================
#define MY_RIGHT_CAL      500     // Your right calibration value (0 degrees)
#define MY_LEFT_CAL       2100    // Your left calibration value (180 degrees)

extern int right_calibration_value;
extern int left_calibration_value;
extern volatile char byte_received;

//=============================================================================
// SCAN CONFIGURATION
//=============================================================================
#define SCAN_START        0
#define SCAN_END          180
#define SCAN_STEP         3        // 3 degree increments (61 points total)
#define NUM_SAMPLES       3        // Number of readings per angle
#define SAMPLE_DELAY_MS   5        // Delay between samples

// Global flag to stop scanning
volatile int stop_scan = 0;

//=============================================================================
// PING DISTANCE MEASUREMENT
//=============================================================================
float get_distance_ping(int angle) {
    cyBOT_Scan_t scan;
    float sum = 0.0f;
    int valid = 0;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        cyBOT_Scan(angle, &scan);
        if (scan.sound_dist > 2.0f && scan.sound_dist < 300.0f) {
            sum += scan.sound_dist;
            valid++;
        }
        timer_waitMillis(SAMPLE_DELAY_MS);
    }

    if (valid > 0) {
        return sum / valid;
    }
    return -1.0f;
}

//=============================================================================
// GET IR READING
//=============================================================================
int get_ir_reading(int angle) {
    cyBOT_Scan_t scan;
    long sum = 0;
    int i;

    for (i = 0; i < NUM_SAMPLES; i++) {
        cyBOT_Scan(angle, &scan);
        sum += scan.IR_raw_val;
        timer_waitMillis(SAMPLE_DELAY_MS);
    }

    return (int)(sum / NUM_SAMPLES);
}

//=============================================================================
// SEND SCAN DATA TO GUI (CSV format for Python)
//=============================================================================
void perform_gui_scan(void) {
    char buf[80];
    int angle;
    float distance_cm;
    int ir_value;

    lcd_printf("GUI Scan");
    stop_scan = 0;  // Reset stop flag

    // Send start marker - Python looks for "START_SCAN"
    uart_sendStr("START_SCAN\n");

    // Send header
    uart_sendStr("Angle,Distance,IR_Raw\n");

    // Perform the scan from 0 to 180 degrees
    for (angle = SCAN_START; angle <= SCAN_END && stop_scan == 0; angle += SCAN_STEP) {
        distance_cm = get_distance_ping(angle);
        ir_value = get_ir_reading(angle);

        // If invalid reading, use max distance (500cm)
        if (distance_cm <= 0) {
            distance_cm = 500.0f;
        }

        // Format: angle,distance,ir_raw\n
        sprintf(buf, "%d,%.2f,%d\n", angle, distance_cm, ir_value);
        uart_sendStr(buf);

        // Small delay to prevent buffer overflow
        timer_waitMillis(10);
    }

    // Send end marker
    if (stop_scan == 1) {
        uart_sendStr("SCAN_STOPPED\n");
        lcd_printf("Scan stopped!");
    } else {
        uart_sendStr("SCAN_COMPLETE\n");
        lcd_printf("Scan done!");
    }
}

//=============================================================================
// MAIN FUNCTION
//=============================================================================
int main(void) {
    // Initialize all systems
    timer_init();
    lcd_init();
    uart_interrupt_init();

    // Initialize CyBot scan with servo, PING, and IR
    cyBOT_init_Scan(0b0111);  // Binary 0111 = servo + PING + IR

    // Set calibration values for servo
    right_calibration_value = MY_RIGHT_CAL;
    left_calibration_value = MY_LEFT_CAL;

    // Send startup message
    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("CyBot GUI Scan Ready\r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("Press 'g' to scan (3 degree steps)\r\n");
    uart_sendStr("Press 's' to stop scanning\r\n");
    uart_sendStr("========================================\r\n");
    lcd_printf("Ready\nPress g or s");

    // Main loop - wait for commands
    while (1) {
        // Check if 'g' was received (start scan)
        if (byte_received == 'g') {
            byte_received = 0;  // Clear the flag

            uart_sendStr("\r\n>>> SCANNING <<<\r\n");
            perform_gui_scan();
            uart_sendStr(">>> SCAN COMPLETE <<<\r\n");
            uart_sendStr("\r\nPress 'g' to scan again\r\n");
            lcd_printf("Scan done!\nPress g");
        }
        // Check if 's' was received (stop scan)
        else if (byte_received == 's') {
            byte_received = 0;
            stop_scan = 1;  // Signal the scan to stop
            uart_sendStr("\r\n>>> STOP COMMAND RECEIVED <<<\r\n");
            lcd_printf("Stopping...");
        }

        timer_waitMillis(10);
    }

    return 0;
}
