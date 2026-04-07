/*
 * movement.h
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */

#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include "open_interface.h"

double move_front(oi_t *sensor_data, double distance_mm);

double move_back(oi_t *sensor_data, double distance_mm);

double turn_right(oi_t*sensor,double degress);

double turn_left(oi_t*sensor,double degress);

int left_bumper(oi_t *sensor_data);

int right_bumper(oi_t *sensor_data);

//int any_bumper(oi_t *sensor_data);

#endif /* MOVEMENT_H_ */
