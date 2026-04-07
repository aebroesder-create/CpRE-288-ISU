/*
 * ping.c
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "ping.h"

// Timer 3B = Vector 52
#define TIMER3B_IRQ     52

volatile uint32_t g_rising_edge_time  = 0;
volatile uint32_t g_falling_edge_time = 0;
volatile bool     g_edge_detected     = false;
volatile uint32_t g_overflow_count    = 0;
volatile uint8_t  g_edge_state        = 0;  // 0 = waiting for rising, 1 = waiting for falling

static void ping_isr(void) {

    if (TIMER3_MIS_R & (1U << 10)) {   // Timer B capture event (bit 10)

        uint32_t captured_value = TIMER3_TBR_R & 0x00FFFFFFU;  // 24-bit counter

        if (g_edge_state == 0) {
            g_rising_edge_time = captured_value;
            g_edge_state = 1;
        } else {
            g_falling_edge_time = captured_value;
            if (g_falling_edge_time > g_rising_edge_time) {  // overflow: timer wrapped
                g_overflow_count++;
            }
            g_edge_detected = true;
            g_edge_state = 0;
        }

        TIMER3_ICR_R = (1U << 10);  // clear Timer B capture interrupt flag
    }
}

void ping_init(void) {

    // Enable clocks for Port B and Timer 3
    SYSCTL_RCGCGPIO_R  |= (1U << 1);
    SYSCTL_RCGCTIMER_R |= (1U << 3);
    while ((SYSCTL_PRGPIO_R  & (1U << 1)) == 0) {}
    while ((SYSCTL_PRTIMER_R & (1U << 3)) == 0) {}

    // PB3 starts as GPIO output for the trigger pulse
    GPIO_PORTB_AFSEL_R &= ~(1U << 3);
    GPIO_PORTB_PCTL_R  &= ~(0xFU << 12);
    GPIO_PORTB_DIR_R   |=  (1U << 3);
    GPIO_PORTB_DEN_R   |=  (1U << 3);
    GPIO_PORTB_DATA_R  &= ~(1U << 3);

    // Disable Timer B before configuring
    TIMER3_CTL_R &= ~(1U << 8);

    // 16-bit split mode - Timer A and B work seperate
    TIMER3_CFG_R = 0x04U;

    // TBMR = 0x07:
    //   bits[1:0] = 0x3  capture mode
    //   bit[2]    = 1    edge-time (not edge-count)
    //   bit[3]    = 0    not PWM
    //   bit[4]    = 0    count-down (errata GPTM#11 workaround)
    TIMER3_TBMR_R = 0x07U;

    // Prescaler + interval load give a 24-bit timer (max 0xFFFFFF)
    TIMER3_TBPR_R  = 0xFFU;
    TIMER3_TBILR_R = 0xFFFFU;

    // Capture both rising and falling edges
    TIMER3_CTL_R = (TIMER3_CTL_R & ~(0x3U << 10)) | (0x3U << 10);

    // Clear stale flags and enable Timer B capture interrupt (CBEIM = bit 10)
    TIMER3_ICR_R  =  (1U << 10);
    TIMER3_IMR_R |=  (1U << 10);

    // Register ISR at runtime
    IntRegister(TIMER3B_IRQ, ping_isr);
    IntEnable(TIMER3B_IRQ);
    IntMasterEnable();
}

void ping_trigger(void) {

    // Switch PB3 to GPIO output, mask interrupt, disable timer before pulse - so don't get unwanted ints for errors
    GPIO_PORTB_AFSEL_R &= ~(1U << 3);
    GPIO_PORTB_PCTL_R  &= ~(0xFU << 12);
    GPIO_PORTB_DIR_R   |=  (1U << 3);
    TIMER3_IMR_R       &= ~(1U << 10);
    TIMER3_CTL_R       &= ~(1U << 8);

    // Send low-high-low start pulse, high must be >= 5 us
    GPIO_PORTB_DATA_R &= ~(1U << 3);
    GPIO_PORTB_DATA_R |=  (1U << 3);
    volatile uint32_t i;
    for (i = 0; i < 80; i++);          // ~5 us at 16 MHz
    GPIO_PORTB_DATA_R &= ~(1U << 3);

    // Reconfig PB3 as T3CCP1 for echo capture
    GPIO_PORTB_AFSEL_R |=  (1U << 3);
    GPIO_PORTB_PCTL_R   = (GPIO_PORTB_PCTL_R & ~(0xFU << 12)) | (0x7U << 12);

    // Clear flags and reset state BEFORE arming capture but AFTER pin config
    // prevents false edges during  pin transition from triggering ISR
    TIMER3_ICR_R        =  (1U << 10);
    g_edge_state        = 0;
    g_edge_detected     = false;
    g_rising_edge_time  = 0;
    g_falling_edge_time = 0;

    // Unmask interrupt and enable timer
    TIMER3_IMR_R |= (1U << 10);
    TIMER3_CTL_R |= (1U << 8);
}


uint32_t ping_get_pulse_width_clocks(void) {

    while (!g_edge_detected) {}

    uint32_t rising  = g_rising_edge_time;
    uint32_t falling = g_falling_edge_time;
    g_edge_detected  = false;

    if (rising >= falling) {
        return (rising - falling);
    } else {
        // Timer wrapped from 0x000000 back to 0xFFFFFF
        return (rising + (0x00FFFFFFU - falling) + 1U);
    }
}

bool ping_overflow_occurred(void) {
    return (g_falling_edge_time > g_rising_edge_time);
}

uint32_t ping_get_overflow_count(void) {
    return g_overflow_count;
}
