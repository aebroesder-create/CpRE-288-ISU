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

// ── UPDATE THESE with your calibration values ───────────────────────────────
#define MY_RIGHT_CAL      500    // from calibration run
#define MY_LEFT_CAL       2100   // from calibration run

// ── Scan settings ───────────────────────────────────────────────────────────
#define SCAN_START        0
#define SCAN_END          180
#define SCAN_STEP         2
#define NUM_IR_SAMPLES    5
#define READ_DELAY_MS     10
#define MAX_POINTS        91
#define MAX_OBJECTS       10

// ── IR threshold ────────────────────────────────────────────────────────────
// Tuned based on max delta from your scan (262, set to half = 130)
#define IR_EDGE_THRESHOLD 130
#define MIN_ANGLE_WIDTH   4

typedef struct { int angle; int ir_raw; } scan_point_t;
scan_point_t scan_data[MAX_POINTS];
int point_count = 0;

typedef struct {
    int id, start_angle, end_angle, center_angle;
} object_t;
object_t objects[MAX_OBJECTS];
int object_count = 0;

int get_avg_ir(int angle, cyBOT_Scan_t *scan) {
    long sum = 0; int i;
    for (i = 0; i < NUM_IR_SAMPLES; i++) {
        cyBOT_Scan(angle, scan);
        sum += scan->IR_raw_val;
        timer_waitMillis(READ_DELAY_MS);
    }
    return (int)(sum / NUM_IR_SAMPLES);
}

void perform_ir_sweep(void) {
    cyBOT_Scan_t r; char buf[80]; int angle;
    point_count = 0;
    uart_sendStr("\r\n=== PART 1: IR SWEEP ===\r\n");
    uart_sendStr("Angle\tIR_Raw\r\n-----\t------\r\n");
    for (angle = SCAN_START; angle <= SCAN_END; angle += SCAN_STEP) {
        if (point_count >= MAX_POINTS) break;
        scan_data[point_count].angle  = angle;
        scan_data[point_count].ir_raw = get_avg_ir(angle, &r);
        sprintf(buf, "%d\t%d\r\n", scan_data[point_count].angle,
                                    scan_data[point_count].ir_raw);
        uart_sendStr(buf);
        point_count++;
    }
    uart_sendStr("=== IR SWEEP COMPLETE ===\r\n");
}

void detect_objects(void) {
    char buf[128]; int i;
    object_count = 0;
    int in_object = 0, start_idx = 0;

    // Print max delta to help tune threshold
    int max_delta = 0;
    for (i = 1; i < point_count; i++) {
        int d = scan_data[i].ir_raw - scan_data[i-1].ir_raw;
        if (d < 0) d = -d;
        if (d > max_delta) max_delta = d;
    }
    sprintf(buf, "\r\nMax IR delta between adjacent angles: %d\r\n"
                 "Current threshold: %d\r\n"
                 "(Objects detected when delta > threshold)\r\n",
            max_delta, IR_EDGE_THRESHOLD);
    uart_sendStr(buf);

    // Show min/max values for reference
    int min_val = scan_data[0].ir_raw;
    int max_val = scan_data[0].ir_raw;
    for (i = 1; i < point_count; i++) {
        if (scan_data[i].ir_raw < min_val) min_val = scan_data[i].ir_raw;
        if (scan_data[i].ir_raw > max_val) max_val = scan_data[i].ir_raw;
    }
    sprintf(buf, "IR Range: Min=%d, Max=%d\r\n", min_val, max_val);
    uart_sendStr(buf);

    uart_sendStr("\r\n=== PART 1: OBJECT DETECTION ===\r\n");
    uart_sendStr("ID  Start  End  Center  Width(deg)\r\n");
    uart_sendStr("--  -----  ---  ------  ----------\r\n");

    for (i = 1; i < point_count; i++) {
        int delta = scan_data[i].ir_raw - scan_data[i-1].ir_raw;

        // Detect rising edge (start of object)
        if (!in_object && delta > IR_EDGE_THRESHOLD) {
            in_object = 1;
            start_idx = i - 1;
        }
        // Detect falling edge (end of object)
        else if (in_object) {
            if ((-delta > IR_EDGE_THRESHOLD) || (i == point_count - 1)) {
                in_object = 0;
                int sa = scan_data[start_idx].angle;
                int ea = scan_data[i].angle;
                int aw = ea - sa;
                if (aw >= MIN_ANGLE_WIDTH && object_count < MAX_OBJECTS) {
                    objects[object_count].id           = object_count + 1;
                    objects[object_count].start_angle  = sa;
                    objects[object_count].end_angle    = ea;
                    objects[object_count].center_angle = (sa + ea) / 2;
                    object_count++;

                    sprintf(buf, "%2d  %3d    %3d  %3d     %3d\r\n",
                            objects[object_count-1].id,
                            objects[object_count-1].start_angle,
                            objects[object_count-1].end_angle,
                            objects[object_count-1].center_angle,
                            objects[object_count-1].end_angle - objects[object_count-1].start_angle);
                    uart_sendStr(buf);
                }
            }
        }
    }

    if (object_count == 0) {
        uart_sendStr("No objects detected.\r\n");
        lcd_printf("No objects");
        return;
    }

    sprintf(buf, "\r\n%d object(s) detected.\r\n", object_count);
    uart_sendStr(buf);
    lcd_printf("%d objects", object_count);
}

int main(void) {
    timer_init();
    oi_t *sensor = oi_alloc();
    oi_init(sensor);
    uart_interrupt_init();
    lcd_init();

    cyBOT_init_Scan(0b0111);
    right_calibration_value = MY_RIGHT_CAL;
    left_calibration_value  = MY_LEFT_CAL;

    uart_sendStr("\r\n=== LAB 7 PART 1 ===\r\n");
    uart_sendStr("IR channel: AIN10 (PB4)\r\n");
    uart_sendStr("Threshold:  130 (half of max delta 262)\r\n");
    uart_sendStr("Press 's' to scan.\r\n");
    lcd_printf("Part 1\nPress s");

    while (1) {
        if (byte_received == 's') {
            byte_received = '\0';
            uart_sendStr("\r\n>>> SCAN START <<<\r\n");
            perform_ir_sweep();
            detect_objects();
            uart_sendStr("\r\nDone. Press 's' to scan again.\r\n");
            lcd_printf("Done\nPress s");
        }
    }
    oi_free(sensor);
    return 0;
}
