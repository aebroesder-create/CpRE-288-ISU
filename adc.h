/*
 * adc.h
 *
 *  Created on: Mar 24, 2026
 *      Author: alecbroe
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void adc_init(void);
uint16_t adc_read(void);
uint16_t adc_read_avg(uint16_t num_samples);
float adc_to_distance(uint16_t adc_val);

#endif /* ADC_H_ */
