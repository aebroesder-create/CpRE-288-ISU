/*
 * cyBot_Scan.c
 *
 *  Created on: Mar 13, 2026
 *      Author: alecbroe
 */

/*
 * cyBot_Scan.c
 *
 *  Created on: Mar 13, 2026
 *      Author: alecbroe
 *      MODIFIED: PING sensor changed from PC4 to PB3
 */

#include "cyBot_Scan.h"
#include "Timer.h"
#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// ── Change this based on diagnostic results ──────────────────────────────────
// Changed from AIN5 to AIN10 (PB4) based on diagnostic test
#define IR_ADC_CHANNEL    10    // AIN10 = PB4 (pin 58)

// ── Servo defaults — overridden by calibration values in main ───────────────
#define DEFAULT_RIGHT_US  500
#define DEFAULT_LEFT_US   2100
#define PING_TIMEOUT_US   30000

// ── Servo speed configuration ────────────────────────────────────────────────
// Number of pulses to send when moving to a new angle
// Original: 5 pulses (~90ms per angle)
// FAST: 2 pulses (~36ms per angle) - 2.5x faster
// ULTRA_FAST: 1 pulse (~18ms per angle) - 5x faster
#define SERVO_PULSE_COUNT_FAST    2    // Fast mode pulses
#define SERVO_PULSE_COUNT_ULTRA   1    // Ultra fast mode pulses
#define SERVO_PULSE_COUNT_NORMAL  5    // Original normal mode

// Current pulse count setting (change this to adjust speed)
#define SERVO_PULSE_COUNT SERVO_PULSE_COUNT_FAST

static int _servo_enabled = 0;
static int _ping_enabled  = 0;
static int _ir_enabled    = 0;

//=============================================================================
// SERVO — PB5
//=============================================================================
static void servo_gpio_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x02;
    while ((SYSCTL_PRGPIO_R & 0x02) == 0);
    GPIO_PORTB_AFSEL_R &= ~0x20;
    GPIO_PORTB_PCTL_R  &= ~0x00F00000;
    GPIO_PORTB_DEN_R   |=  0x20;
    GPIO_PORTB_DIR_R   |=  0x20;
    GPIO_PORTB_DATA_R  &= ~0x20;
}

static void servo_pulse_us(int pulse_us) {
    if (pulse_us <  500) pulse_us =  500;
    if (pulse_us > 2500) pulse_us = 2500;
    GPIO_PORTB_DATA_R |=  0x20;
    timer_waitMicros(pulse_us);
    GPIO_PORTB_DATA_R &= ~0x20;
    timer_waitMillis(18);  // 20ms period - cannot reduce this due to servo timing
}

static void servo_set_angle(int angle) {
    if (!_servo_enabled) return;
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;

    int right_us = (right_calibration_value > 0) ? right_calibration_value : DEFAULT_RIGHT_US;
    int left_us  = (left_calibration_value  > 0) ? left_calibration_value  : DEFAULT_LEFT_US;
    int pulse_us = right_us + ((left_us - right_us) * angle) / 180;

    // Send pulses to move servo - fewer pulses = faster movement
    int p;
    for (p = 0; p < SERVO_PULSE_COUNT; p++) servo_pulse_us(pulse_us);
}

//=============================================================================
// PING — PB3 (changed from PC4 based on diagnostic test)
//=============================================================================
static void ping_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x02;      // Enable Port B clock (bit 1)
    while ((SYSCTL_PRGPIO_R & 0x02) == 0);
    GPIO_PORTB_AFSEL_R &= ~0x08;    // Disable alternate function on PB3 (bit 3)
    GPIO_PORTB_DEN_R   |=  0x08;    // Enable digital on PB3
    GPIO_PORTB_DIR_R   |=  0x08;    // Set as output initially
    GPIO_PORTB_DATA_R  &= ~0x08;    // Start low
}

static float ping_get_distance(void) {
    if (!_ping_enabled) return -1.0f;
    int timeout;

    // Send trigger pulse on PB3
    GPIO_PORTB_DIR_R   |=  0x08;    // Set as output
    GPIO_PORTB_AFSEL_R &= ~0x08;    // Disable alternate function
    GPIO_PORTB_DATA_R  &= ~0x08;    // Start low
    timer_waitMicros(2);            // 2us low
    GPIO_PORTB_DATA_R  |=  0x08;    // High
    timer_waitMicros(5);            // 5us high
    GPIO_PORTB_DATA_R  &= ~0x08;    // Back to low

    // Switch to input to listen for echo
    GPIO_PORTB_DIR_R &= ~0x08;      // Set as input
    timer_waitMicros(750);          // Holdoff per PING datasheet

    // Wait for echo to start (pin goes high)
    timeout = 5000;
    while (!(GPIO_PORTB_DATA_R & 0x08)) {
        timer_waitMicros(1);
        if (--timeout == 0) return -1.0f;
    }

    // Measure echo pulse width
    uint32_t echo_us = 0;
    timeout = 20000;
    while (GPIO_PORTB_DATA_R & 0x08) {
        timer_waitMicros(1);
        echo_us++;
        if (--timeout == 0) return -1.0f;
    }

    // Convert microseconds to cm (speed of sound: 58us per cm round trip)
    return (float)echo_us / 58.0f;
}

//=============================================================================
// IR — AIN10 (PB4) via ADC0 SS3
//=============================================================================
static void ir_init(void) {
    // Enable Port B clock for PB4 (AIN10)
    SYSCTL_RCGCGPIO_R |= 0x02;  // Port B clock
    while ((SYSCTL_PRGPIO_R & 0x02) == 0);

    // Configure PB4 (pin 58) as analog input
    GPIO_PORTB_AMSEL_R |= 0x10;   // Enable analog on PB4
    GPIO_PORTB_DEN_R   &= ~0x10;   // Disable digital on PB4
    GPIO_PORTB_AFSEL_R &= ~0x10;   // Disable alternate function
    GPIO_PORTB_DIR_R   &= ~0x10;   // Make input

    // Enable ADC0 clock
    SYSCTL_RCGCADC_R |= 0x01;
    timer_waitMillis(5);

    // Configure ADC0 SS3 for single sample
    ADC0_ACTSS_R  &= ~0x08;
    ADC0_EMUX_R   &= ~0xF000;
    ADC0_SSMUX3_R  = IR_ADC_CHANNEL;
    ADC0_SSCTL3_R  = 0x06;
    ADC0_ACTSS_R  |=  0x08;
}

static int ir_read_raw(void) {
    if (!_ir_enabled) return -1;
    ADC0_PSSI_R = 0x08;
    while ((ADC0_RIS_R & 0x08) == 0);
    int v = ADC0_SSFIFO3_R & 0xFFF;
    ADC0_ISC_R = 0x08;
    return v;
}

//=============================================================================
// PUBLIC API
//=============================================================================
void cyBOT_init_Scan(int feature) {
    _servo_enabled = (feature & 0x01) ? 1 : 0;
    _ping_enabled  = (feature & 0x02) ? 1 : 0;
    _ir_enabled    = (feature & 0x04) ? 1 : 0;

    right_calibration_value = DEFAULT_RIGHT_US;
    left_calibration_value  = DEFAULT_LEFT_US;

    if (_servo_enabled) servo_gpio_init();
    if (_ping_enabled)  ping_init();
    if (_ir_enabled)    ir_init();

    // Long startup sweep to ensure servo reaches 0 degrees
    if (_servo_enabled) {
        int p;
        int right_us = (right_calibration_value > 0) ?
                        right_calibration_value : DEFAULT_RIGHT_US;
        for (p = 0; p < 25; p++) servo_pulse_us(right_us);
    }
}

void cyBOT_Scan(int angle, cyBOT_Scan_t *getScan) {
    servo_set_angle(angle);
    if (getScan == NULL) return;
    getScan->sound_dist = _ping_enabled ? ping_get_distance() : -1.0f;
    getScan->IR_raw_val = _ir_enabled   ? ir_read_raw()       : -1;
}

unsigned int cyBOT_scan_version(void) { return 3232026; }

cyBOT_SERVRO_cal_t cyBOT_SERVO_cal(void) {
    cyBOT_SERVRO_cal_t cal;
    cal.right = _servo_enabled ? right_calibration_value : -1;
    cal.left  = _servo_enabled ? left_calibration_value  : -1;
    return cal;
}
