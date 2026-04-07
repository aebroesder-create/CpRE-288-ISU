/*
 * ping.c
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#include "ping.h"
#include "inc/tm4c123gh6pm.h"
#include <stdbool.h>

// Global variables for interrupt handler
volatile uint32_t g_rising_edge_time = 0;
volatile uint32_t g_falling_edge_time = 0;
volatile bool g_edge_detected = false;
volatile uint32_t g_overflow_count = 0;

// Timer overflow interrupt handler for Timer 3B
void Timer3B_Handler(void) {
    // Check for capture event (rising or falling edge)
    if(TIMER3->MIS & TIMER_MIS_CAEMIS) {
        // Get the captured value
        uint32_t captured_value = TIMER3->TBR;

        // Check if this is rising or falling edge
        // We need to track the state - using a simple state machine
        static uint8_t edge_state = 0;

        if(edge_state == 0) {
            // Rising edge detected
            g_rising_edge_time = captured_value;
            edge_state = 1;
        } else {
            // Falling edge detected
            g_falling_edge_time = captured_value;
            g_edge_detected = true;
            edge_state = 0;
        }

        // Clear the capture event flag
        TIMER3->ICR = TIMER_ICR_CAECINT;
    }

    // Check for timer overflow
    if(TIMER3->RIS & TIMER_RIS_TATORIS) {
        g_overflow_count++;
        // Clear overflow flag
        TIMER3->ICR = TIMER_ICR_TATOCINT;
    }
}

void ping_init(void) {
    // Enable clock for GPIO Port B and Timer 3
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R1;  // Enable GPIO Port B clock
    SYSCTL->RCGCTIMER |= SYSCTL_RCGCTIMER_R2; // Enable Timer 3 clock

    // Small delay for clock stabilization
    volatile uint32_t delay = SYSCTL->RCGCGPIO;
    delay = SYSCTL->RCGCTIMER;

    // Initialize PB3 for trigger pulse (output)
    GPIOB->DIR |= (1 << 3);     // Set PB3 as output
    GPIOB->DEN |= (1 << 3);     // Enable digital function
    GPIOB->AFSEL &= ~(1 << 3);  // Disable alternate function for now
    GPIOB->PCTL &= ~(0xF << 12); // Clear PCTL bits for PB3

    // Disable Timer 3 during configuration
    TIMER3->CTL &= ~(TIMER_CTL_TBEN);

    // Configure Timer 3B for input edge-time mode
    TIMER3->CFG = TIMER_CFG_16_BIT;  // 16-bit mode

    // Configure Timer B: Input edge-time mode, count-down
    TIMER3->TAMR = 0;  // Clear Timer A settings
    TIMER3->TBMR = TIMER_TBMR_TBMR_CAP |    // Capture mode
                   TIMER_TBMR_TBCMR |        // Capture on both edges
                   TIMER_TBMR_TBAMS;         // Input edge-time mode

    // Set prescaler and interval load values for 24-bit timer
    TIMER3->TBILR = 0xFFFF;     // Interval load low 16 bits
    TIMER3->TBPR = 0xFF;         // Prescaler value

    // Configure capture control
    TIMER3->CTL &= ~(TIMER_CTL_TBEVENT_M);
    TIMER3->CTL |= TIMER_CTL_TBEVENT_BOTH;  // Capture on both edges

    // Configure interrupt
    TIMER3->IMR |= TIMER_IMR_CAEIM;   // Enable capture event interrupt
    TIMER3->IMR |= TIMER_IMR_TATOIM;  // Enable overflow interrupt

    // Enable Timer 3 interrupt in NVIC
    NVIC_ENABLE0 |= (1 << 19);  // Timer 3 interrupt number 19

    // Enable Timer 3B
    TIMER3->CTL |= TIMER_CTL_TBEN;

    // Global interrupt enable
    __asm(" CPSIE I");
}

void ping_trigger(void) {
    // Disable Timer 3 and its interrupt during trigger
    TIMER3->CTL &= ~(TIMER_CTL_TBEN);

    // Configure PB3 as GPIO output for trigger
    GPIOB->AFSEL &= ~(1 << 3);  // Disable alternate function
    GPIOB->DEN |= (1 << 3);     // Enable digital
    GPIOB->DIR |= (1 << 3);     // Set as output

    // Generate 5 microsecond pulse (adjust delay for your clock speed)
    GPIOB->DATA |= (1 << 3);     // Set high
    for(volatile int i = 0; i < 10; i++);  // Small delay (~5us at 80MHz)
    GPIOB->DATA &= ~(1 << 3);    // Set low

    // Reconfigure PB3 as Timer 3B CCP input
    GPIOB->AFSEL |= (1 << 3);    // Enable alternate function
    GPIOB->PCTL |= (0x7 << 12);  // Set PB3 to Timer 3 CCP1 function

    // Reset edge detection state
    g_edge_detected = false;
    g_rising_edge_time = 0;
    g_falling_edge_time = 0;

    // Reset timer for new capture
    TIMER3->TBV = 0xFFFFFF;      // Start at top value

    // Enable Timer 3B
    TIMER3->CTL |= TIMER_CTL_TBEN;
}

uint32_t ping_getPulseWidthClocks(void) {
    // Wait for edge detection
    while(!g_edge_detected);

    uint32_t rising = g_rising_edge_time;
    uint32_t falling = g_falling_edge_time;
    uint32_t overflow = g_overflow_count;

    // Reset for next measurement
    g_edge_detected = false;

    // Calculate pulse width accounting for overflow
    uint32_t pulse_width_clocks;

    if(falling > rising) {
        pulse_width_clocks = falling - rising;
    } else {
        // Overflow occurred - timer wrapped around
        pulse_width_clocks = (0xFFFFFF - rising) + falling;
    }

    return pulse_width_clocks;
}

float ping_getPulseWidthMs(void) {
    uint32_t clocks = ping_getPulseWidthClocks();
    // At 80 MHz system clock, each clock cycle = 12.5 ns
    return (clocks * 12.5e-6);  // Convert to milliseconds
}

float ping_getDistance(void) {
    float pulse_width_ms = ping_getPulseWidthMs();

    // Speed of sound = 343 m/s = 0.0343 cm/us
    // Distance = (time * speed) / 2 (round trip)
    // Convert ms to seconds, multiply by 34300 cm/s, divide by 2
    float distance_cm = (pulse_width_ms / 1000.0) * 34300.0 / 2.0;

    return distance_cm;
}
