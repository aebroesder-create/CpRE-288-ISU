/*
 * cyBot_Scan.c
 *
 *  Created on: Mar 13, 2026
 *      Author: alecbroe
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
    timer_waitMillis(18);
}

static void servo_set_angle(int angle) {
    if (!_servo_enabled) return;
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;

    int right_us = (right_calibration_value > 0) ? right_calibration_value : DEFAULT_RIGHT_US;
    int left_us  = (left_calibration_value  > 0) ? left_calibration_value  : DEFAULT_LEFT_US;
    int pulse_us = right_us + ((left_us - right_us) * angle) / 180;

    // 5 pulses = ~100ms per step — fast enough for 2-degree sweep
    int p;
    for (p = 0; p < 5; p++) servo_pulse_us(pulse_us);
}

//=============================================================================
// PING — PC4
//=============================================================================
static void ping_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x04;
    while ((SYSCTL_PRGPIO_R & 0x04) == 0);
    GPIO_PORTC_AFSEL_R &= ~0x10;
    GPIO_PORTC_DEN_R   |=  0x10;
    GPIO_PORTC_DIR_R   |=  0x10;
    GPIO_PORTC_DATA_R  &= ~0x10;
}

static float ping_get_distance(void) {
    if (!_ping_enabled) return -1.0f;
    int timeout;

    GPIO_PORTC_DIR_R   |=  0x10;
    GPIO_PORTC_AFSEL_R &= ~0x10;
    GPIO_PORTC_DATA_R  &= ~0x10;
    timer_waitMicros(2);
    GPIO_PORTC_DATA_R  |=  0x10;
    timer_waitMicros(5);
    GPIO_PORTC_DATA_R  &= ~0x10;

    GPIO_PORTC_DIR_R &= ~0x10;
    timer_waitMicros(750);  // holdoff per PING datasheet

    timeout = 5000;
    while (!(GPIO_PORTC_DATA_R & 0x10)) {
        timer_waitMicros(1);
        if (--timeout == 0) return -1.0f;
    }

    uint32_t echo_us = 0;
    timeout = 20000;
    while (GPIO_PORTC_DATA_R & 0x10) {
        timer_waitMicros(1);
        echo_us++;
        if (--timeout == 0) return -1.0f;
    }
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
