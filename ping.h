/*
 * ping.h
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#ifndef PING_H_
#define PING_H_

// Globals shared with ISR
extern volatile uint32_t g_rising_edge_time;
extern volatile uint32_t g_falling_edge_time;
extern volatile bool     g_edge_detected;
extern volatile uint32_t g_overflow_count;
extern volatile uint8_t  g_edge_state;

void     ping_init(void);
void     ping_trigger(void);
uint32_t ping_get_pulse_width_clocks(void);
bool     ping_overflow_occurred(void);
uint32_t ping_get_overflow_count(void);

#endif
