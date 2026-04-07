/*
 * CyBotCalibration.c
 *
 *  Created on: Mar 13, 2026
 *      Author: alecbroe
 */


#include "cyBot_Scan.h"
#include "open_interface.h"
#include "Timer.h"
#include "lcd.h"
#include "button.h"
#include "uart-interrupt.h"
#include "movement.h"
#include <stdio.h>
#include <math.h>

// ── helpers ──────────────────────────────────────────────────────────────────

static char msg[128];

static void print(const char *s) {
    uart_sendStr(s);
}

static void println(const char *s) {
    uart_sendStr(s);
    uart_sendStr("\r\n");
}

static void printline(void) {
    println("--------------------------------------------");
}

/* Block until 's' is received over UART */
static void wait_for_s(void) {
    println("  >> Press 's' in PuTTY to continue...");
    while (byte_received != 's') { timer_waitMillis(50); }
    byte_received = 0;
}

/* Block until any key is received, return it */
static char wait_for_key(void) {
    byte_received = 0;
    while (byte_received == 0) { timer_waitMillis(50); }
    char c = byte_received;
    byte_received = 0;
    return c;
}

// ── SECTION 1: SERVO CALIBRATION ─────────────────────────────────────────────
/*
 * Calibration values are in MICROSECONDS — simple, human-readable numbers.
 *
 *   0 deg  (right) = ~1000 us (1.0 ms pulse)
 *   90 deg (center)= ~1500 us (1.5 ms pulse)
 *   180 deg(left)  = ~2000 us (2.0 ms pulse)
 *
 * Keys in PuTTY:
 *   'a' = move LEFT  (increase us)
 *   'd' = move RIGHT (decrease us)
 *   'w' = double step size (coarse)
 *   'x' = halve step size  (fine)
 *   's' = confirm
 */

/* Send one servo pulse of pulse_us microseconds on PB5 */
static void servo_pulse_us(int pulse_us) {
    if (pulse_us <  500) pulse_us =  500;
    if (pulse_us > 2500) pulse_us = 2500;
    (*((volatile unsigned long *)0x40005080)) |=  0x20;  // PB5 HIGH
    timer_waitMicros(pulse_us);
    (*((volatile unsigned long *)0x40005080)) &= ~0x20;  // PB5 LOW
    timer_waitMillis(18);
}

static void servo_hold_us(int pulse_us, int pulses) {
    int p;
    for (p = 0; p < pulses; p++) servo_pulse_us(pulse_us);
}

static int calibrate_servo_endpoint(const char *label, int start_us,
                                    int min_us, int max_us) {
    int  val  = start_us;
    int  step = 50;
    char key;

    snprintf(msg, sizeof(msg),
        "\r\n  === Calibrating: %s ===\r\n"
        "  Values in microseconds. Range: %d to %d us.\r\n"
        "  a=LEFT(+)  d=RIGHT(-)  w=step x2  x=step /2  s=CONFIRM\r\n",
        label, min_us, max_us);
    print(msg);

    servo_hold_us(val, 10);

    while (1) {
        snprintf(msg, sizeof(msg),
            "  pulse = %4d us   step = %3d us   (s to confirm)\r",
            val, step);
        print(msg);

        key = wait_for_key();

        if (key == 'a') {
            val += step;
            if (val > max_us) { val = max_us; print("\r\n  [At max limit]\r\n"); }
        } else if (key == 'd') {
            val -= step;
            if (val < min_us) { val = min_us; print("\r\n  [At min limit]\r\n"); }
        } else if (key == 'w') {
            step *= 2; if (step > 200) step = 200;
            snprintf(msg, sizeof(msg), "\r\n  Step: %d us\r\n", step); print(msg);
        } else if (key == 'x') {
            step /= 2; if (step < 5) step = 5;
            snprintf(msg, sizeof(msg), "\r\n  Step: %d us\r\n", step); print(msg);
        } else if (key == 's') {
            break;
        }

        servo_hold_us(val, 8);
    }

    snprintf(msg, sizeof(msg), "\r\n  CONFIRMED: %s = %d us\r\n", label, val);
    println(msg);
    return val;
}

static void section_servo(void) {
    printline();
    println("=== SECTION 1: SERVO CALIBRATION ===");
    println("  Values are in MICROSECONDS (us) — small numbers!");
    println("    0 deg  (right) = ~1000 us");
    println("   90 deg  (center)= ~1500 us");
    println("  180 deg  (left)  = ~2000 us");
    println("  Use a/d to nudge, w/x to change step, s to confirm.");
    printline();
    wait_for_s();

    println("\r\n  STEP 1 of 2 — Set servo to 0 degrees (far RIGHT)");
    println("  Starting at 1000 us. Press d to move right if needed.");
    int right_val = calibrate_servo_endpoint(
        "right_calibration_value (0 deg)", 1000, 500, 1400);

    println("\r\n  STEP 2 of 2 — Set servo to 180 degrees (far LEFT)");
    println("  Starting at 2000 us. Press a to move left if needed.");
    int left_val = calibrate_servo_endpoint(
        "left_calibration_value (180 deg)", 2000, 1600, 2500);

    printline();
    println("  *** SERVO CALIBRATION COMPLETE ***");
    snprintf(msg, sizeof(msg),
        "  right_calibration_value = %d\r\n"
        "  left_calibration_value  = %d\r\n",
        right_val, left_val);
    print(msg);
    println("  Copy these into lab7.c after cyBOT_init_Scan().");
    printline();

    right_calibration_value = right_val;
    left_calibration_value  = left_val;

    println("  Running sweep: 0 -> 90 -> 180 -> 0 ...");
    cyBOT_Scan(0,   NULL); timer_waitMillis(500);
    cyBOT_Scan(90,  NULL); timer_waitMillis(500);
    cyBOT_Scan(180, NULL); timer_waitMillis(500);
    cyBOT_Scan(0,   NULL); timer_waitMillis(500);
    println("  Did it reach both ends? If not, re-run this section.");

    lcd_clear();
    snprintf(msg, sizeof(msg), "R=%dus", right_val);
    lcd_printf("%s\nL=%dus", msg, left_val);

    wait_for_s();
}

// ── SECTION 2: MOVEMENT CALIBRATION ──────────────────────────────────────────
/*
 * Drives the bot known distances / angles so you can measure actual vs commanded.
 * Adjust the calibration multipliers at the top of movement.c if needed.
 *
 * Tests:
 *   A) Forward 500mm — measure with ruler
 *   B) Backward 500mm — should return to start
 *   C) Right turn 90° — should face 90° clockwise
 *   D) Left turn 90°  — should return to original heading
 */

static void section_movement(oi_t *sensor) {
    printline();
    println("=== SECTION 2: MOVEMENT CALIBRATION ===");
    println("  Place the bot on a flat surface with clear space.");
    println("  Mark the starting position with tape.");
    println("  Each test will prompt you before moving.");
    printline();
    wait_for_s();

    double result;

    // Test A: Forward 500mm
    println("\n  TEST A — Moving FORWARD 500mm");
    println("  Watch the bot. Measure how far it actually travels.");
    wait_for_s();
    result = move_front(sensor, 500.0);
    snprintf(msg, sizeof(msg), "  move_front returned: %.1f mm\r\n", result);
    print(msg);
    println("  Measure actual distance traveled. If not 500mm,");
    println("  adjust the distance multiplier in movement.c.");
    wait_for_s();

    // Test B: Backward 500mm
    println("\n  TEST B — Moving BACKWARD 500mm (return to start)");
    wait_for_s();
    result = move_back(sensor, 500.0);
    snprintf(msg, sizeof(msg), "  move_back returned: %.1f mm\r\n", result);
    print(msg);
    println("  Bot should be back at the tape mark.");
    wait_for_s();

    // Test C: Right turn 90 degrees
    println("\n  TEST C — Turning RIGHT 90 degrees");
    println("  Place a reference mark straight ahead before confirming.");
    wait_for_s();
    result = turn_right(sensor, 90.0);
    snprintf(msg, sizeof(msg), "  turn_right returned: %.1f deg\r\n", result);
    print(msg);
    println("  Bot should now face 90 degrees clockwise from the mark.");
    println("  If not accurate, adjust the turn multiplier in movement.c.");
    wait_for_s();

    // Test D: Left turn 90 degrees (restore heading)
    println("\n  TEST D — Turning LEFT 90 degrees (restore original heading)");
    wait_for_s();
    result = turn_left(sensor, 90.0);
    snprintf(msg, sizeof(msg), "  turn_left returned: %.1f deg\r\n", result);
    print(msg);
    println("  Bot should now face the original reference mark again.");
    wait_for_s();

    printline();
    println("  *** MOVEMENT CALIBRATION COMPLETE ***");
    println("  Adjust multipliers in movement.c if turns/distances were off.");
    printline();
    wait_for_s();
}

// ── SECTION 3: PING (ULTRASONIC) CALIBRATION ─────────────────────────────────
/*
 * Points the servo to 90 degrees (straight ahead) and takes 10 PING readings.
 * Place an object at a known distance (e.g. 30cm, measured with a ruler)
 * and verify the readings match.
 *
 * If readings are consistently off by a fixed factor, the echo timing
 * divisor in cyBot_Scan.c (currently /58.0) may need adjustment.
 * A higher divisor → shorter reported distance.
 * A lower divisor  → longer reported distance.
 */

static void section_ping(void) {
    printline();
    println("=== SECTION 3: PING (ULTRASONIC) CALIBRATION ===");
    println("  Place a flat object (e.g. a book) at exactly 30cm");
    println("  directly in front of the PING sensor.");
    println("  The servo will point to 90 degrees (straight ahead).");
    printline();
    wait_for_s();

    cyBOT_Scan_t scan;

    // Point straight ahead and settle
    println("  Pointing servo to 90 degrees...");
    cyBOT_Scan(90, &scan);
    timer_waitMillis(500);

    println("\n  Taking 10 PING readings (target = 30.0 cm):");
    println("  Reading   Distance(cm)");
    println("  -------   ------------");

    float sum = 0.0f;
    int i;
    for (i = 1; i <= 10; i++) {
        cyBOT_Scan(90, &scan);
        float d = scan.sound_dist;
        sum += (d > 0) ? d : 0;
        snprintf(msg, sizeof(msg), "  %4d      %8.2f cm\r\n", i, d);
        print(msg);
        timer_waitMillis(200);
    }

    float avg = sum / 10.0f;
    snprintf(msg, sizeof(msg), "\n  Average: %.2f cm  (expected: 30.00 cm)\r\n", avg);
    print(msg);

    if (avg > 0.1f) {
        float correction = 30.0f / avg;
        snprintf(msg, sizeof(msg),
            "  Correction factor: %.4f\r\n"
            "  If average != 30.0, change the divisor in ping_get_distance()\r\n"
            "  in cyBot_Scan.c from /58.0 to /%.2f\r\n",
            correction, 58.0f / correction);
        print(msg);
    }

    printline();
    println("  *** PING CALIBRATION COMPLETE ***");
    printline();
    wait_for_s();
}

// ── SECTION 4: IR SENSOR CALIBRATION ─────────────────────────────────────────
/*
 * Shows live IR ADC readings while you hold objects at different distances.
 * Helps you determine the correct IR_EDGE_THRESHOLD for object detection.
 *
 * The Sharp GP2D12 IR sensor:
 *   - Higher ADC value = object closer / present
 *   - Lower ADC value  = no object / far away
 *   - Open air (no object) typically reads 200–600
 *   - Object at 30cm typically reads 1500–2500
 *   - Object at 10cm typically reads 2500–3500
 *
 * Recommended threshold = midpoint between your open-air reading
 * and your typical-object reading.
 */

static void section_ir(void) {
    printline();
    println("=== SECTION 4: IR SENSOR CALIBRATION ===");
    println("  The servo will stay at 90 degrees (straight ahead).");
    println("  Watch the live ADC readings as you move objects in and out.");
    println("");
    println("  STEP 1: Note the reading with NO object in front (open air).");
    println("  STEP 2: Note the reading with an object at ~30cm.");
    println("  STEP 3: Set IR_EDGE_THRESHOLD = midpoint of those two values.");
    println("");
    println("  Press any key to PAUSE the live feed.");
    println("  Press 's' to exit this section.");
    printline();
    wait_for_s();

    cyBOT_Scan_t scan;
    int min_seen = 4095, max_seen = 0;
    int reading_count = 0;

    println("\n  Live IR readings (Ctrl+C in PuTTY or press 's' to stop):\n");

    byte_received = 0;
    while (1) {
        // Check for keypress
        if (byte_received != 0) {
            char k = byte_received;
            byte_received = 0;
            if (k == 's') break;
            // Any other key = pause until next keypress
            println("\n  [PAUSED — press any key to resume]");
            wait_for_key();
            println("  [RESUMED]");
        }

        cyBOT_Scan(90, &scan);
        int ir = scan.IR_raw_val;

        if (ir < min_seen) min_seen = ir;
        if (ir > max_seen) max_seen = ir;
        reading_count++;

        // Simple bar graph (each '#' = 100 ADC units)
        int bars = ir / 100;
        if (bars > 40) bars = 40;
        char bar[42];
        int b;
        for (b = 0; b < bars; b++) bar[b] = '#';
        bar[bars] = '\0';

        snprintf(msg, sizeof(msg), "  IR=%4d |%-40s|\r\n", ir, bar);
        print(msg);

        timer_waitMillis(150);
    }

    // Summary
    printline();
    println("  *** IR CALIBRATION SUMMARY ***");
    snprintf(msg, sizeof(msg),
        "  Minimum reading seen: %d\r\n"
        "  Maximum reading seen: %d\r\n",
        min_seen, max_seen);
    print(msg);

    if (max_seen > min_seen + 200) {
        int threshold = (min_seen + max_seen) / 2;
        snprintf(msg, sizeof(msg),
            "  Suggested IR_EDGE_THRESHOLD = %d\r\n"
            "  (midpoint between min and max)\r\n",
            threshold);
        print(msg);
        snprintf(msg, sizeof(msg), "IR Thresh=%d", threshold);
        lcd_printf("%s", msg);
    } else {
        println("  Warning: min and max are very close — sensor may not be");
        println("  detecting objects. Check wiring (PE3/AIN0 or PE2/AIN1).");
    }

    printline();
    wait_for_s();
}

// ── MAIN ──────────────────────────────────────────────────────────────────────

int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();

    lcd_printf("CyBot\nCalibration");

    // Initialize scan library (servo + PING + IR all enabled)
    cyBOT_init_Scan(0b0111);

    // Set default calibration values — will be overwritten by servo cal section
    right_calibration_value = 1000;  // microseconds
    left_calibration_value  = 2000;  // microseconds

    // Initialize OI for movement tests
    oi_t *sensor = oi_alloc();
    oi_init(sensor);

    // Welcome
    printline();
    println("  CyBot All-In-One Calibration Program");
    println("  CpE 288 — Iowa State University");
    printline();
    println("  This program calibrates:");
    println("    1. Servo motor");
    println("    2. Movement (drive + turns)");
    println("    3. PING ultrasonic sensor");
    println("    4. IR distance sensor");
    println("");
    println("  Write down all values shown — you will need them in lab7.c.");
    printline();
    wait_for_s();

    // Run all four calibration sections
    section_servo();
    section_movement(sensor);
    section_ping();
    section_ir();

    // Final summary
    printline();
    println("  === ALL CALIBRATION COMPLETE ===");
    println("  Values to copy into lab7.c:");
    snprintf(msg, sizeof(msg),
        "  right_calibration_value = %d\r\n"
        "  left_calibration_value  = %d\r\n",
        right_calibration_value, left_calibration_value);
    print(msg);
    println("  IR_EDGE_THRESHOLD = (value noted from Section 4)");
    printline();
    println("  Reset the board and flash lab7.c when ready.");

    lcd_printf("Cal Done!\nReset bot");

    oi_free(sensor);
    return 0;
}
