/*
 * lab7.c
 *
 *  Created on: Mar 10, 2026
 *      Author: alecbroe
 */

/*
 * lab7.c
 *
 *  Created on: Mar 10, 2026
 *      Author: alecbroe
 */

#include "cyBot_Scan.h"
#include "open_interface.h"
#include "movement.h"
#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

//=============================================================================
// CALIBRATION VALUES
//=============================================================================
#define MY_RIGHT_CAL      500     // working value for 0 degrees
#define MY_LEFT_CAL       2100    // working value for 180 degrees

//=============================================================================
// SCAN CONFIGURATION
//=============================================================================
#define SCAN_START        0
#define SCAN_END          180
#define SCAN_STEP         3           // For IR detection
#define GUI_SCAN_STEP     4           // For GUI scan (4 degree increments)
#define NUM_IR_SAMPLES    3
#define NUM_PING_SAMPLES  3
#define READ_DELAY_MS     5
#define MAX_POINTS        61
#define MAX_OBJECTS       10

//=============================================================================
// OBJECT DETECTION PARAMETERS
//=============================================================================
#define IR_EDGE_THRESHOLD 130
#define MIN_ANGLE_WIDTH   9
#define MIN_IR_PEAK       800
#define STOP_DIST_CM      10.0
#define BUMP_AVOID_DIST   150.0

//=============================================================================
// TIMING
//=============================================================================
#define BUMP_CHECK_DELAY_MS  10
#define DISTANCE_CHECK_DELAY_MS 15
#define POST_TURN_DELAY_MS   100
#define POST_ACTION_DELAY_MS  50

//=============================================================================
// DATA STRUCTURES
//=============================================================================
typedef struct {
    int angle;
    int ir_raw;
} scan_point_t;

scan_point_t scan_data[MAX_POINTS];
int point_count = 0;

typedef struct {
    int id;
    int start_angle;
    int end_angle;
    int center_angle;
    float distance_cm;
    float linear_width_cm;
    int peak_ir_raw;
} object_t;

object_t objects[MAX_OBJECTS];
int object_count = 0;

// Global variables
extern int right_calibration_value;
extern int left_calibration_value;
extern volatile char byte_received;
int handling_bump = 0;

//=============================================================================
// FUNCTION PROTOTYPES
//=============================================================================
void perform_scan_and_detect(void);
void perform_gui_scan(void);
int find_smallest_by_width(void);
void navigate_to_smallest(oi_t *sensor);
void handle_bump(oi_t *sensor, object_t *target);
int check_for_bump(oi_t *sensor);
float get_distance_ping(int angle, cyBOT_Scan_t *scan);
int get_avg_ir(int angle, cyBOT_Scan_t *scan);
float ir_raw_to_distance_cm(int ir_raw);

//=============================================================================
// IR TO DISTANCE CONVERSION (IF PING FAILS)
//=============================================================================
float ir_raw_to_distance_cm(int ir_raw) {
    if (ir_raw <= 200) return 80.0f;
    if (ir_raw <= 300) return 70.0f;
    if (ir_raw <= 400) return 60.0f;
    if (ir_raw <= 500) return 50.0f;
    if (ir_raw <= 600) return 40.0f;
    if (ir_raw <= 700) return 35.0f;
    if (ir_raw <= 800) return 30.0f;
    if (ir_raw <= 900) return 25.0f;
    if (ir_raw <= 1000) return 20.0f;
    if (ir_raw <= 1100) return 17.0f;
    if (ir_raw <= 1200) return 14.0f;
    if (ir_raw <= 1300) return 12.0f;
    if (ir_raw <= 1400) return 10.0f;
    return 8.0f;
}

//=============================================================================
// PING DISTANCE MEASUREMENT
//=============================================================================
float get_distance_ping(int angle, cyBOT_Scan_t *scan) {
    float sum = 0.0f;
    int valid = 0;
    int i;
    float dist;

    for (i = 0; i < NUM_PING_SAMPLES; i++) {
        cyBOT_Scan(angle, scan);
        dist = scan->sound_dist;

        if (dist > 2.0f && dist < 300.0f) {
            sum += dist;
            valid++;
        }
        timer_waitMillis(READ_DELAY_MS);
    }

    if (valid > 0) {
        return sum / valid;
    }
    return -1.0f;
}

//=============================================================================
// IR READING WITH AVERAGING
//=============================================================================
int get_avg_ir(int angle, cyBOT_Scan_t *scan) {
    long sum = 0;
    int i;
    for (i = 0; i < NUM_IR_SAMPLES; i++) {
        cyBOT_Scan(angle, scan);
        sum += scan->IR_raw_val;
        timer_waitMillis(READ_DELAY_MS);
    }
    return (int)(sum / NUM_IR_SAMPLES);
}

//=============================================================================
// CHECK FOR BUMP
//=============================================================================
int check_for_bump(oi_t *sensor) {
    oi_update(sensor);

    if (sensor->bumpLeft || sensor->bumpRight) {
        char buf[100];
        sprintf(buf, "\r\nBUMP DETECTED! Left=%d, Right=%d\r\n",
                sensor->bumpLeft, sensor->bumpRight);
        uart_sendStr(buf);
        return 1;
    }
    return 0;
}

//=============================================================================
// HANDLE BUMP
//=============================================================================
void handle_bump(oi_t *sensor, object_t *target) {
    char buf[200];
    int bump_side = 0;
    int i;

    handling_bump = 1;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("!!! BUMP DETECTED - AVOIDING OBSTACLE !!!\r\n");
    uart_sendStr("========================================\r\n");

    oi_update(sensor);

    if (sensor->bumpLeft && sensor->bumpRight) {
        bump_side = 2;
        uart_sendStr("Both bump sensors triggered!\r\n");
    } else if (sensor->bumpLeft) {
        bump_side = 0;
        uart_sendStr("Left bump sensor triggered!\r\n");
    } else if (sensor->bumpRight) {
        bump_side = 1;
        uart_sendStr("Right bump sensor triggered!\r\n");
    }

    oi_setWheels(0, 0);
    timer_waitMillis(POST_ACTION_DELAY_MS);

    uart_sendStr("Backing up...\r\n");
    move_back(sensor, BUMP_AVOID_DIST);
    timer_waitMillis(POST_ACTION_DELAY_MS);

    if (bump_side == 0) {
        uart_sendStr("Turning right to avoid obstacle...\r\n");
        turn_right(sensor, 45);
    } else if (bump_side == 1) {
        uart_sendStr("Turning left to avoid obstacle...\r\n");
        turn_left(sensor, 45);
    } else {
        uart_sendStr("Bumped head-on - turning left...\r\n");
        turn_left(sensor, 45);
    }

    timer_waitMillis(POST_ACTION_DELAY_MS);

    uart_sendStr("Moving forward to clear obstacle...\r\n");
    move_front(sensor, BUMP_AVOID_DIST);
    timer_waitMillis(POST_ACTION_DELAY_MS);

    uart_sendStr("Turning back toward original direction...\r\n");
    if (bump_side == 0) {
        turn_left(sensor, 45);
    } else if (bump_side == 1) {
        turn_right(sensor, 45);
    } else {
        turn_right(sensor, 45);
    }

    timer_waitMillis(POST_ACTION_DELAY_MS);

    uart_sendStr("\r\nQuick rescan to locate target...\r\n");

    object_count = 0;
    point_count = 0;
    perform_scan_and_detect();

    if (object_count > 0) {
        int found = 0;
        for (i = 0; i < object_count; i++) {
            if (objects[i].id == target->id) {
                target->center_angle = objects[i].center_angle;
                target->distance_cm = objects[i].distance_cm;
                target->linear_width_cm = objects[i].linear_width_cm;
                found = 1;
                uart_sendStr("Found original target!\r\n");
                break;
            }
        }

        if (!found) {
            uart_sendStr("Original target lost - finding smallest object...\r\n");
            int new_idx = find_smallest_by_width();
            if (new_idx >= 0) {
                *target = objects[new_idx];
                uart_sendStr("New target selected\r\n");
            }
        }
    }

    sprintf(buf, "New target: Object %d at %d° at %.1f cm\r\n",
            target->id, target->center_angle, target->distance_cm);
    uart_sendStr(buf);

    timer_waitMillis(POST_ACTION_DELAY_MS);
    handling_bump = 0;
}

//=============================================================================
// SCAN AND DETECT OBJECTS (ORIGINAL LAB7 FUNCTION)
//=============================================================================
void perform_scan_and_detect(void) {
    cyBOT_Scan_t scan;
    char buf[200];
    int angle;
    int i, j;
    int temp_start[MAX_OBJECTS];
    int temp_end[MAX_OBJECTS];
    int temp_peak[MAX_OBJECTS];
    int temp_count;
    int in_obj;
    int start_idx;
    int delta;
    int sa, ea, width_deg, peak, center;
    float distance;
    double half_angle_rad;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("SCANNING FOR OBJECTS \r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("SCAN_START\r\n");
    uart_sendStr("Angle\tIR_Raw\r\n");
    uart_sendStr("-----\t------\r\n");

    point_count = 0;
    for (angle = SCAN_START; angle <= SCAN_END; angle += SCAN_STEP) {
        if (point_count >= MAX_POINTS) break;
        scan_data[point_count].angle = angle;
        scan_data[point_count].ir_raw = get_avg_ir(angle, &scan);
        sprintf(buf, "%d\t%d\r\n", scan_data[point_count].angle,
                scan_data[point_count].ir_raw);
        uart_sendStr(buf);
        point_count++;
    }

    uart_sendStr("\r\n--- DETECTING OBJECT EDGES ---\r\n");

    temp_count = 0;
    in_obj = 0;

    for (i = 1; i < point_count; i++) {
        delta = scan_data[i].ir_raw - scan_data[i-1].ir_raw;

        if (!in_obj && delta > IR_EDGE_THRESHOLD) {
            in_obj = 1;
            start_idx = i-1;
            sprintf(buf, ">>> OBJECT START at %d°\r\n", scan_data[start_idx].angle);
            uart_sendStr(buf);
        }
        else if (in_obj) {
            if ((-delta > IR_EDGE_THRESHOLD) || (i == point_count-1)) {
                in_obj = 0;
                sa = scan_data[start_idx].angle;
                ea = scan_data[i].angle;
                width_deg = ea - sa;

                peak = scan_data[start_idx].ir_raw;
                for (j = start_idx; j <= i; j++) {
                    if (scan_data[j].ir_raw > peak) peak = scan_data[j].ir_raw;
                }

                if (width_deg >= MIN_ANGLE_WIDTH && temp_count < MAX_OBJECTS) {
                    temp_start[temp_count] = sa;
                    temp_end[temp_count] = ea;
                    temp_peak[temp_count] = peak;
                    temp_count++;
                    sprintf(buf, ">>> OBJECT END at %d° (width: %d°)\r\n", ea, width_deg);
                    uart_sendStr(buf);
                }
            }
        }
    }

    uart_sendStr("\r\n--- MEASURING DISTANCES WITH PING ---\r\n");

    object_count = 0;
    for (i = 0; i < temp_count; i++) {
        width_deg = temp_end[i] - temp_start[i];
        center = (temp_start[i] + temp_end[i]) / 2;
        peak = temp_peak[i];

        if (peak > MIN_IR_PEAK && width_deg >= MIN_ANGLE_WIDTH) {
            distance = get_distance_ping(center, &scan);

            if (distance <= 0) {
                distance = ir_raw_to_distance_cm(peak);
                sprintf(buf, "Object %d: PING failed, using IR estimate (%.1f cm)\r\n",
                        object_count+1, distance);
                uart_sendStr(buf);
            } else {
                sprintf(buf, "Object %d: PING measured (%.1f cm)\r\n",
                        object_count+1, distance);
                uart_sendStr(buf);
            }

            objects[object_count].id = object_count + 1;
            objects[object_count].start_angle = temp_start[i];
            objects[object_count].end_angle = temp_end[i];
            objects[object_count].center_angle = center;
            objects[object_count].distance_cm = distance;
            objects[object_count].peak_ir_raw = peak;

            half_angle_rad = (width_deg / 2.0) * M_PI / 180.0;
            objects[object_count].linear_width_cm =
                2.0 * distance * tan(half_angle_rad);

            object_count++;
        }
    }

    if (object_count == 0) {
        uart_sendStr("\r\nNo objects detected.\r\n");
        lcd_printf("No objects");
        return;
    }

    uart_sendStr("\r\n--- OBJECT RESULTS ---\r\n");
    uart_sendStr("ID  Center  Dist(cm)  Width(cm)\r\n");
    uart_sendStr("--  ------  --------  ---------\r\n");

    for (j = 0; j < object_count; j++) {
        sprintf(buf, "%2d    %3d      %6.1f     %7.2f\r\n",
                objects[j].id, objects[j].center_angle,
                objects[j].distance_cm, objects[j].linear_width_cm);
        uart_sendStr(buf);
    }

    for (j = 0; j < object_count; j++) {
        sprintf(buf, "DATA:%d,%.2f,%d,%d\r\n",
                objects[j].center_angle,
                objects[j].distance_cm,
                objects[j].start_angle,
                objects[j].end_angle);
        uart_sendStr(buf);
    }
    uart_sendStr("SCAN_END\r\n");

    sprintf(buf, "\r\nTotal objects detected: %d\r\n", object_count);
    uart_sendStr(buf);
    lcd_printf("%d objects", object_count);
}

//=============================================================================
// GUI SCAN MODE - NEW FUNCTION FOR PYTHON GUI
// Sends data in "angle,distance\n" format (comma-separated)
//=============================================================================
void perform_gui_scan(void) {
    cyBOT_Scan_t scan;
    char buf[50];
    int angle;
    float distance_cm;

    uart_sendStr("SCAN_START\n");

    for (angle = 0; angle <= 180; angle += GUI_SCAN_STEP) {
        distance_cm = get_distance_ping(angle, &scan);

        if (distance_cm <= 0) {
            distance_cm = 500.0f;  // No obstacle = max distance
        }

        // Format: "angle,distance\n" - exactly what Python GUI expects
        sprintf(buf, "%.0f,%.0f\n", (float)angle, distance_cm);
        uart_sendStr(buf);
        timer_waitMillis(10);
    }

    uart_sendStr("SCAN_COMPLETE\n");
}

//=============================================================================
// FIND SMALLEST OBJECT BY LINEAR WIDTH
//=============================================================================
int find_smallest_by_width(void) {
    int best;
    int i;

    if (object_count == 0) return -1;
    best = 0;
    for (i = 1; i < object_count; i++) {
        if (objects[i].linear_width_cm < objects[best].linear_width_cm) {
            best = i;
        }
    }
    return best;
}

//=============================================================================
// NAVIGATE TO SMALLEST OBJECT
//=============================================================================
void navigate_to_smallest(oi_t *sensor) {
    char buf[200];
    cyBOT_Scan_t scan;
    int idx;
    object_t tgt;
    float current_distance;
    int turn_degrees;
    int samples;
    int k;
    int attempts;
    int drive_speed;
    int bump_detected;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("NAVIGATING TO SMALLEST OBJECT\r\n");
    uart_sendStr("========================================\r\n");

    if (object_count == 0) {
        uart_sendStr("No objects to navigate to!\r\n");
        lcd_printf("No objects");
        return;
    }

    idx = find_smallest_by_width();
    tgt = objects[idx];

    sprintf(buf,
        "\r\nTARGET: Object %d at %d° | Distance: %.1f cm | Width: %.2f cm (SMALLEST)\r\n",
        tgt.id, tgt.center_angle, tgt.distance_cm, tgt.linear_width_cm);
    uart_sendStr(buf);
    lcd_printf("Target: Obj%d", tgt.id);

    uart_sendStr("\r\nStep 1: Centering servo to 90°...\r\n");
    cyBOT_Scan(90, &scan);
    timer_waitMillis(POST_TURN_DELAY_MS);

    if(tgt.center_angle > 90) {
        turn_degrees = tgt.center_angle - 90;
        sprintf(buf, "\r\nStep 2: Turning LEFT %d degrees to face target...\r\n", turn_degrees);
        uart_sendStr(buf);
        turn_left(sensor, turn_degrees);
    } else {
        turn_degrees = 90 - tgt.center_angle;
        sprintf(buf, "\r\nStep 2: Turning RIGHT %d degrees to face target...\r\n", turn_degrees);
        uart_sendStr(buf);
        turn_right(sensor, turn_degrees);
    }

    uart_sendStr("Turn complete - Robot now facing target\r\n");
    timer_waitMillis(POST_TURN_DELAY_MS);

    uart_sendStr("\r\nStep 3: Getting initial distance measurement...\r\n");

    current_distance = 0;
    samples = 0;
    for (k = 0; k < 5; k++) {
        float dist = get_distance_ping(90, &scan);
        if (dist > 0 && dist < 200) {
            current_distance += dist;
            samples++;
        }
        timer_waitMillis(READ_DELAY_MS);
    }
    if (samples > 0) {
        current_distance = current_distance / samples;
    } else {
        current_distance = tgt.distance_cm;
        uart_sendStr("Using stored distance (PING temporarily unavailable)\r\n");
    }

    sprintf(buf, "Initial distance to target: %.1f cm\r\n", current_distance);
    uart_sendStr(buf);

    uart_sendStr("\r\nStep 4: Driving toward target (will stop at 10cm)...\r\n");
    uart_sendStr("Bump sensors active - will avoid obstacles immediately\r\n");

    float STOP_BUFFER = 12.0;
    float TARGET_STOP = 10.0;

    if (current_distance <= TARGET_STOP + 2.0) {
        uart_sendStr("Already within 12cm of target! No driving needed.\r\n");
        lcd_printf("At target!");
        return;
    }

    drive_speed = 150;
    oi_setWheels(drive_speed, drive_speed);

    attempts = 0;
    int max_attempts = 80;
    bump_detected = 0;

    while (current_distance > TARGET_STOP && attempts < max_attempts) {
        oi_update(sensor);

        if ((sensor->bumpLeft || sensor->bumpRight) && !handling_bump) {
            bump_detected = 1;

            oi_setWheels(0, 0);
            timer_waitMillis(10);
            handle_bump(sensor, &tgt);

            cyBOT_Scan(90, &scan);
            timer_waitMillis(POST_TURN_DELAY_MS);

            if(tgt.center_angle > 90) {
                turn_degrees = tgt.center_angle - 90;
                turn_left(sensor, turn_degrees);
            } else {
                turn_degrees = 90 - tgt.center_angle;
                turn_right(sensor, turn_degrees);
            }

            timer_waitMillis(POST_TURN_DELAY_MS);

            current_distance = 0;
            samples = 0;
            for (k = 0; k < 3; k++) {
                float dist = get_distance_ping(90, &scan);
                if (dist > 0 && dist < 200) {
                    current_distance += dist;
                    samples++;
                }
                timer_waitMillis(READ_DELAY_MS);
            }
            if (samples > 0) {
                current_distance = current_distance / samples;
            }

            sprintf(buf, "New distance: %.1f cm\r\n", current_distance);
            uart_sendStr(buf);

            attempts = 0;
            drive_speed = 150;

            if (current_distance <= TARGET_STOP) {
                break;
            }

            oi_setWheels(drive_speed, drive_speed);
            continue;
        }

        timer_waitMillis(BUMP_CHECK_DELAY_MS);

        current_distance = 0;
        samples = 0;
        for (k = 0; k < 2; k++) {
            float dist = get_distance_ping(90, &scan);
            if (dist > 0 && dist < 200) {
                current_distance += dist;
                samples++;
            }
            timer_waitMillis(DISTANCE_CHECK_DELAY_MS);
        }
        if (samples > 0) {
            current_distance = current_distance / samples;
        }

        if (current_distance > 0 && current_distance < 200) {
            if (attempts % 10 == 0) {
                sprintf(buf, "Distance: %.1f cm", current_distance);
                uart_sendStr(buf);

                if (current_distance < STOP_BUFFER) {
                    int new_speed = (int)(150 * (current_distance - 3.0) / STOP_BUFFER);
                    if (new_speed < 50) new_speed = 50;
                    if (new_speed > 150) new_speed = 150;

                    if (new_speed != drive_speed) {
                        drive_speed = new_speed;
                        sprintf(buf, " - Slowing to %d speed\r\n", drive_speed);
                        uart_sendStr(buf);
                        oi_setWheels(drive_speed, drive_speed);
                    } else {
                        uart_sendStr("\r\n");
                    }
                } else {
                    uart_sendStr("\r\n");
                }
            }

            if (current_distance <= TARGET_STOP) {
                uart_sendStr("\r\nTarget distance reached! Stopping...\r\n");
                break;
            }
        }

        attempts++;
    }

    oi_setWheels(0, 0);
    timer_waitMillis(POST_ACTION_DELAY_MS);

    uart_sendStr("\r\nTaking final distance measurement...\r\n");
    current_distance = 0;
    samples = 0;
    for (k = 0; k < 3; k++) {
        float dist = get_distance_ping(90, &scan);
        if (dist > 0 && dist < 200) {
            current_distance += dist;
            samples++;
        }
        timer_waitMillis(READ_DELAY_MS);
    }
    if (samples > 0) {
        current_distance = current_distance / samples;
    }

    sprintf(buf, "\r\nFinal distance to target: %.1f cm\r\n", current_distance);
    uart_sendStr(buf);

    if (bump_detected) {
        uart_sendStr("\r\n*** Navigation completed with bump avoidance! ***\r\n");
    }

    if (current_distance <= 12.0) {
        uart_sendStr("\r\nSUCCESS: Robot stopped within 12cm of target!\r\n");
    } else {
        uart_sendStr("\r\nWARNING: Robot stopped farther than desired. Check sensors.\r\n");
    }

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("MISSION COMPLETE!\r\n");
    uart_sendStr("========================================\r\n");
    lcd_printf("Complete!\n%.0fcm", current_distance);
}

//=============================================================================
// MAIN FUNCTION
//=============================================================================
int main(void) {
    oi_t *sensor;

    timer_init();
    sensor = oi_alloc();
    oi_init(sensor);
    uart_interrupt_init();
    lcd_init();

    // Initialize CyBot scan with servo, PING, and IR
    cyBOT_init_Scan(0b0111);  // Binary 0111 = servo + PING + IR

    // Set calibration values
    right_calibration_value = MY_RIGHT_CAL;
    left_calibration_value = MY_LEFT_CAL;

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("LAB 7 - FULL VERSION\r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("Press 'g' for GUI scan (PING only for Python GUI)\r\n");
    uart_sendStr("Press 'm' for full mission (IR detection + driving)\r\n");
    uart_sendStr("========================================\r\n");
    lcd_printf("Press g or m");

    while (1) {
        // GUI SCAN MODE - For Python GUI (sends comma-separated data)
        if (byte_received == 'g') {
            byte_received = 0;
            uart_sendStr("\r\n>>> GUI SCAN START <<<\r\n");
            lcd_printf("GUI Scan");
            perform_gui_scan();
            uart_sendStr(">>> GUI SCAN END <<<\r\n");
            lcd_printf("Scan done!\nPress g or m");
        }
        // FULL MISSION MODE - Original lab7 functionality
        else if (byte_received == 'm') {
            byte_received = 0;
            handling_bump = 0;

            uart_sendStr("\r\n>>> MISSION START <<<\r\n");
            perform_scan_and_detect();

            if (object_count > 0) {
                navigate_to_smallest(sensor);
            } else {
                uart_sendStr("No objects detected. Place objects in field.\r\n");
                lcd_printf("No objects");
            }
            uart_sendStr("\r\nPress 'g' for GUI scan, or 'm' for mission.\r\n");
            lcd_printf("Done!\nPress g or m");
        }
        timer_waitMillis(10);
    }

    oi_free(sensor);
    return 0;
}
