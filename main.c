/*
 * main.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */
/******************************************************************************
 * LAB 3
 *
 * One command does it all: Press 's' to scan, detect, and point to smallest object
 ******************************************************************************/

#include "cyBot_uart.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "Timer.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//=============================================================================
// CONSTANTS
//=============================================================================
#define SCAN_START      0
#define SCAN_END        180
#define SCAN_STEP       2
#define NUM_READINGS    3
#define READ_DELAY_MS   10

#define MAX_POINTS      100
#define MAX_OBJECTS     10
#define VALID_DIST_MIN  2
#define VALID_DIST_MAX  250
#define OBJECT_DIST_MAX 100
#define MIN_OBJECT_WIDTH 4

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================
extern int right_calibration_value;
extern int left_calibration_value;

//=============================================================================
// DATA STRUCTURES
//=============================================================================
typedef struct {
    int angle;
    float distance;
    int valid;
} scan_point_t;

scan_point_t scan_points[MAX_POINTS];
int point_count = 0;

typedef struct {
    int id;
    int center_angle;
    float distance;
    int width;
} object_t;

object_t objects[MAX_OBJECTS];
int object_count = 0;

//=============================================================================
// UART FUNCTIONS
//=============================================================================
void sendString(char *str) {
    int i;  // DECLARE OUTSIDE LOOP
    for (i = 0; i < strlen(str); i++) {
        cyBot_sendByte(str[i]);
    }
}

//=============================================================================
// PING SENSOR FUNCTIONS
//=============================================================================
float get_average_distance(int angle, cyBOT_Scan_t *scan) {
    float sum = 0;
    int i;  // DECLARE OUTSIDE LOOP
    for (i = 0; i < NUM_READINGS; i++) {
        cyBOT_Scan(angle, scan);
        sum += scan->sound_dist;
        timer_waitMillis(READ_DELAY_MS);
    }
    return sum / NUM_READINGS;
}

//=============================================================================
// SCAN FUNCTION
//=============================================================================
void perform_scan(void) {
    cyBOT_Scan_t scan_result;
    char buffer[100];
    int angle;
    int count = 0;

    point_count = 0;

    sendString("\n\r========================================\n\r");
    sendString("SCAN RESULTS\n\r");
    sendString("========================================\n\r");

    // Print table header
    sendString("\n\rIndex\tAngle\tDistance\tStatus\n\r");
    sendString("-----\t-----\t--------\t------\n\r");

    for (angle = SCAN_START; angle <= SCAN_END; angle += SCAN_STEP) {
        float dist = get_average_distance(angle, &scan_result);

        if (point_count < MAX_POINTS) {
            scan_points[point_count].angle = angle;
            scan_points[point_count].distance = dist;
            scan_points[point_count].valid = (dist >= VALID_DIST_MIN && dist <= VALID_DIST_MAX);
            point_count++;
        }

        // Print each reading in table format
        sprintf(buffer, "%d\t%d\t%.1f\t\t%s\n\r",
                count + 1,
                angle,
                dist,
                (dist >= VALID_DIST_MIN && dist <= VALID_DIST_MAX) ? "Valid" : "Invalid");
        sendString(buffer);

        count++;
    }

    sendString("========================================\n\r");

    // Print quick summary
    sprintf(buffer, "Total: %d readings\n\r\n\r", point_count);
    sendString(buffer);
}

//=============================================================================
// OBJECT DETECTION FUNCTION
//=============================================================================
void detect_objects(void) {
    int i;  // DECLARE OUTSIDE LOOP
    int in_object = 0;
    int start_idx = 0;
    float sum_dist = 0;
    int valid_count = 0;
    char buffer[200];

    object_count = 0;

    for (i = 0; i < point_count; i++) {
        int is_object = (scan_points[i].valid && scan_points[i].distance < OBJECT_DIST_MAX);

        if (!in_object && is_object) {
            in_object = 1;
            start_idx = i;
            sum_dist = scan_points[i].distance;
            valid_count = 1;
        }
        else if (in_object) {
            if (is_object) {
                sum_dist += scan_points[i].distance;
                valid_count++;
            }

            int end_of_object = 0;
            if (!is_object) end_of_object = 1;
            if (i == point_count - 1) end_of_object = 1;
            if (i > 0 && scan_points[i].distance > scan_points[i-1].distance + 15) end_of_object = 1;

            if (end_of_object) {
                in_object = 0;
                int width = abs(scan_points[i-1].angle - scan_points[start_idx].angle);

                if (width >= MIN_OBJECT_WIDTH && object_count < MAX_OBJECTS) {
                    objects[object_count].id = object_count + 1;
                    objects[object_count].center_angle =
                        (scan_points[start_idx].angle + scan_points[i-1].angle) / 2;
                    objects[object_count].distance = sum_dist / valid_count;
                    objects[object_count].width = width;
                    object_count++;
                }
            }
        }
    }

    // Show results
    if (object_count > 0) {
        int j;  // DECLARE OUTSIDE LOOP
        sendString("\n\rOBJECTS FOUND:\n\r");
        for (j = 0; j < object_count; j++) {
            sprintf(buffer, " %d@%d:%d ", objects[j].id, objects[j].center_angle, objects[j].width);
            sendString(buffer);
        }
        sendString("\n\r");
    } else {
        sendString("\n\rNO OBJECTS\n\r");
    }
}

//=============================================================================
// POINT TO SMALLEST FUNCTION
//=============================================================================
void point_to_smallest(void) {
    int i;  // DECLARE OUTSIDE LOOP

    if (object_count == 0) {
        sendString("No target\n\r");
        lcd_printf("No object");
        return;
    }

    int smallest = 0;
    for (i = 1; i < object_count; i++) {
        if (objects[i].width < objects[smallest].width) {
            smallest = i;
        }
    }

    char buffer[100];
    sprintf(buffer, "Smallest: obj%d@%d\n\r", objects[smallest].id, objects[smallest].center_angle);
    sendString(buffer);

    cyBOT_Scan(objects[smallest].center_angle, NULL);
    lcd_printf("Smallest:%d", objects[smallest].center_angle);
}

//=============================================================================
// MAIN FUNCTION - SIMPLIFIED
//=============================================================================
int main(void) {
    // Initialize hardware
    oi_t *sensor = oi_alloc();
    oi_init(sensor);
    cyBot_uart_init();
    lcd_init();

    cyBOT_init_Scan(0b0011);

    // Your calibration values
    right_calibration_value = 311500;
    left_calibration_value = 1309000;

    cyBOT_Scan_t *scan_struct = calloc(1, sizeof(cyBOT_Scan_t));

    // Simple welcome message
    sendString("\n\rLAB 3 READY\n\r");
    sendString("Press 's' to scan\n\r");
    lcd_printf("Press s to scan");

    char cmd;

    // ONE COMMAND LOOP - only responds to 's'
    while (1) {
        cmd = cyBot_getByte();  // Wait for key press

        if (cmd == 's') {
            sendString("\n\r>>> STARTING SCAN <<<\n\r");

            // DO EVERYTHING
            perform_scan();      // 1. Scan
            detect_objects();    // 2. Detect objects
            point_to_smallest(); // 3. Point to smallest

            sendString("\n\rPress 's' to scan again\n\r");
        }
        // Ignore all other keys
    }

    // This code is unreachable, but kept for completeness
    free(scan_struct);
    oi_free(sensor);
    return 0;
}
