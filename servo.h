/*
 * servo.h
 *
 *  Created on: Apr 6, 2026
 *      Author: alecbroe
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

void servo_init(void);
void servo_move(uint16_t degrees);

#endif /* SERVO_H_ */
