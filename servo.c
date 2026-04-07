/*
 * servo.c
 *
 *  Created on: Apr 6, 2026
 *      Author: alecbroe
 */

#include "servo.h"
#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

#define PERIOD_COUNTS 320000   // 20 ms @ 16MHz
#define COUNTS_PER_US 16

void servo_init(void) {
    // 1. Clock Gating: Port B and Timer 1
    SYSCTL_RCGCGPIO_R |= 0x02;
    SYSCTL_RCGCTIMER_R |= 0x02;
    while((SYSCTL_PRGPIO_R & 0x02) == 0);
    while((SYSCTL_PRTIMER_R & 0x02) == 0);

    // 2. Configure PB5 for T1CCP1 (Timer 1B)
    GPIO_PORTB_AFSEL_R |= 0x20;
    GPIO_PORTB_PCTL_R &= ~0x00F00000;
    GPIO_PORTB_PCTL_R |=  0x00700000;
    GPIO_PORTB_DEN_R |= 0x20;
    GPIO_PORTB_DIR_R |= 0x20;

    // 3. Configure Timer 1B for PWM
    TIMER1_CTL_R &= ~0x0100;             // Disable Timer 1B
    TIMER1_CFG_R = 0x04;                 // 16-bit configuration

    // TBAMS=1 (PWM), TBCMR=0 (Edge), TBMR=2 (Periodic)
    TIMER1_TBMR_R = 0x0A;
    TIMER1_TBMR_R &= ~0x0010;            // Ensure Down-count

    // 4. Set Period (320,000)
    // ILR holds lower 16 bits, TBPR holds upper 8 bits (prescaler)
    TIMER1_TBILR_R = (PERIOD_COUNTS & 0xFFFF);
    TIMER1_TBPR_R = (PERIOD_COUNTS >> 16) & 0xFF;

    // 5. Default position 90 degrees
    servo_move(90);

    // 6. Enable Timer 1B
    TIMER1_CTL_R |= 0x0100;
}

void servo_move(uint16_t degrees) {
    if (degrees > 180) degrees = 180;

    // Pulse width: 1ms (0 deg) to 2ms (180 deg)
    // 1ms = 16,000 counts. 2ms = 32,000 counts.
    uint32_t high_pulse_counts = 16000 + (degrees * 16000 / 180);

    // Match Value = Period - High Time
    uint32_t match_value = PERIOD_COUNTS - high_pulse_counts;

    // Set Match Register (lower 16 bits) and Match Prescale (upper 8 bits)
    TIMER1_TBMATCHR_R = (match_value & 0xFFFF);
    TIMER1_TBPMR_R = (match_value >> 16) & 0xFF;
}
