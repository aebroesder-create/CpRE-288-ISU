/*
 * movement.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */

#include "open_interface.h"
#include "movement.h"
#include "Timer.h"
#include "lcd.h"


double move_front(oi_t *sensor_data, double distance_mm) {
    double forward_distance = 0.0;
    double target = fabs(distance_mm);
    int bump_count = 0;

    oi_setWheels(200, 200);

    while (forward_distance < target) {
        oi_update(sensor_data);

        // Handle bumps FIRST and do NOT credit this iteration's distance
        // (prevents encoder slip while stuck from counting as real progress).
        if (sensor_data->bumpRight || sensor_data->bumpLeft) {
            bump_count++;
            oi_setWheels(0, 0);

            if (sensor_data->bumpRight) {
                double back = move_back(sensor_data, 150.0); // back is negative
                forward_distance += back;                    // discount backup from progress

                turn_left(sensor_data, 90.0);
                move_front(sensor_data, 250.0);
                turn_right(sensor_data, 90.0);
            }
            else if (sensor_data->bumpLeft) {
                double back = move_back(sensor_data, 150.0); // back is negative
                forward_distance += back;

                turn_right(sensor_data, 90.0);
                move_front(sensor_data, 250.0);
                turn_left(sensor_data, 90.0);
            }

            oi_setWheels(200, 200);
            timer_waitMillis(10);
            continue; // skip distance accumulation for this loop iteration
        }

        // Normal forward tracking (include signed distance)
        forward_distance += sensor_data->distance;

        timer_waitMillis(10);
    }

    oi_setWheels(0, 0);
    return forward_distance;
}


double move_back(oi_t *sensor_data, double distance_mm) {
    double sum = 0.0;
    double target;

    // Make target positive for comparison
    if (distance_mm < 0) {
        target = -distance_mm;
    } else {
        target = distance_mm;
    }

    // Start moving backward (negative speeds)
    oi_setWheels(-200, -200);

    // Move until target distance is reached
    while (1) {
        oi_update(sensor_data);
        sum += sensor_data->distance;  // Will be negative when moving backward

        // Calculate absolute value of sum
        double abs_sum;
        if (sum < 0) {
            abs_sum = -sum;
        } else {
            abs_sum = sum;
        }

        // Check if we've traveled enough
        if (abs_sum >= target) {
            break;
        }

        // Small delay
        timer_waitMillis(10);
    }

    // Stop
    oi_setWheels(0, 0);

    return sum;
}

double turn_right(oi_t *sensor_data, double degrees) {
    double angle_sum = 0.0;
    double target;
   degrees = degrees*0.95;
    // Make target positive for comparison
    target = fabs(degrees);

    // Start turning LEFT
    // Left wheel backward, right wheel forward = counter-clockwise turn
    oi_setWheels(-150, 150);  // Moderate turn speed

    // Turn until target angle is reached
    while (1) {
        oi_update(sensor_data);
        angle_sum += sensor_data->angle;  // Angle accumulates during turns

        // Note: sensor_data->angle is usually POSITIVE when turning LEFT
        // But check your Roomba documentation to be sure!

        // Check if we've turned enough (use absolute value)
        if (fabs(angle_sum) >= target) {
            break;
        }

        // Small delay
        timer_waitMillis(10);
    }

    // Stop
    oi_setWheels(0, 0);

    return angle_sum;  // Returns actual angle turned (usually positive for left)
}

double turn_left(oi_t *sensor_data, double degrees) {
    double angle_sum = 0.0;
    double target;
degrees = degrees*0.96;
    // Make target positive for comparison
    target = fabs(degrees);

    // Start turning RIGHT
    // Left wheel forward, right wheel backward = clockwise turn
    oi_setWheels(150, -150);  // Moderate turn speed

    // Turn until target angle is reached
    while (1) {
        oi_update(sensor_data);
        angle_sum += sensor_data->angle;  // Angle accumulates during turns

        // Note: sensor_data->angle is usually NEGATIVE when turning RIGHT
        // But check your Roomba documentation to be sure!

        // Check if we've turned enough (use absolute value)
        if (fabs(angle_sum) >= target) {
            break;
        }

        // Small delay
        timer_waitMillis(10);
    }

    // Stop
    oi_setWheels(0, 0);

    return angle_sum;  // Returns actual angle turned (usually negative for right)
}
