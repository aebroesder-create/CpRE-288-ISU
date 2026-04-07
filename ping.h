/*
 * ping.h
 *
 *  Created on: Mar 30, 2026
 *      Author: alecbroe
 */

#ifndef PING_H_
#define PING_H_

#include <stdint.h>
#include <stdbool.h>

// Function prototypes
void ping_init(void);
void ping_trigger(void);
float ping_getDistance(void);
uint32_t ping_getPulseWidthClocks(void);
float ping_getPulseWidthMs(void);

#endif /* PING_H_ */
