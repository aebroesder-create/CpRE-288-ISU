/*
 * adc.c
 *
 *  Created on: Mar 24, 2026
 *      Author: alecbroe
 */

#include "adc.h"
#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

#define TABLE_SIZE 16

typedef struct {
    uint16_t adc_val;
    float    dist_cm;
} CalibPoint;

static const CalibPoint cal_table[TABLE_SIZE] = {

    {2690, 12.0f},
    {2440, 13.5f},
    {2200, 15.0f},
    {2030, 17.0f},
    {1800, 20.0f},
    {1680, 22.0f},
    {1520, 25.0f},
    {1430, 27.0f},
    {1300, 30.0f},
    {1170, 33.0f},
    {1100, 35.0f},
    {1000, 38.0f},
    {950, 40.0f},
    {890, 42.0f},
    {810, 45.0f},
    {725, 50.0f}
};

void adc_init(void) {
    volatile uint32_t delay;

    // Enable ADC0 and Port B clocks, then read to stabilize
    SYSCTL_RCGCADC_R  |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x02;
    delay = SYSCTL_RCGCGPIO_R;

    // Configure PB4 as analog input for AIN10
    GPIO_PORTB_DIR_R   &= ~0x10;
    GPIO_PORTB_AFSEL_R |=  0x10;
    GPIO_PORTB_DEN_R   &= ~0x10;
    GPIO_PORTB_AMSEL_R |=  0x10;


    ADC0_ACTSS_R &= ~0x08;  // Disable SS3


    ADC0_EMUX_R &= ~0xF000;  // Clear bits 15-12 for processor trigger


    ADC0_SSMUX3_R = 10;      // AIN10 = 10

    // 0x06 = binary 0110: IE0=1 (interrupt enable), END0=1 (end of sequence)
    ADC0_SSCTL3_R = 0x06;

    ADC0_SAC_R = 0x04;       // 0x04 = average 16 samples

    // Re-enable SS3
    ADC0_ACTSS_R |= 0x08;
}

uint16_t adc_read(void) {
    // Clear prior completion flag, then convert
    ADC0_ISC_R  = 0x08;      // Clear SS3 interrupt
    ADC0_PSSI_R = 0x08;      // Start SS3 conversion

    // Wait for conversion to finish
    while ((ADC0_RIS_R & 0x08) == 0) {}

    uint16_t result = ADC0_SSFIFO3_R & 0xFFF;  // Mask to 12 bits
    ADC0_ISC_R = 0x08;       // Clear interrupt again
    return result;
}

uint16_t adc_read_avg(uint16_t num_samples) {
    uint32_t sum = 0;
    uint16_t i;
    for (i = 0; i < num_samples; i++) {
        sum += adc_read();
    }
    return (uint16_t)(sum / num_samples);
}

float adc_to_distance(uint16_t adc_val) {
    // Clamp to table boundaries
    if (adc_val >= cal_table[0].adc_val)             return cal_table[0].dist_cm;
    if (adc_val <= cal_table[TABLE_SIZE-1].adc_val)  return cal_table[TABLE_SIZE-1].dist_cm;

    // Find surrounding entries and linearly interpolate
    uint8_t i;
    for (i = 0; i < TABLE_SIZE - 1; i++) {
        // ADC values decrease as distance increases
        if (adc_val <= cal_table[i].adc_val && adc_val >= cal_table[i+1].adc_val) {
            float x0 = (float)cal_table[i].adc_val;
            float y0 = cal_table[i].dist_cm;
            float x1 = (float)cal_table[i+1].adc_val;
            float y1 = cal_table[i+1].dist_cm;
            // Linear interpolation formula
            return y0 + (y1 - y0) * ((float)adc_val - x0) / (x1 - x0);
        }
    }
    return -1.0f;  // Error case
}
