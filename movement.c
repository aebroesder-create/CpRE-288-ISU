/*
 * movement.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */

/*
 * movement.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */

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

//=============================================================================
// TURN CALIBRATION CONSTANTS
//=============================================================================
// Start with these values, you'll calibrate later
#define MS_PER_DEGREE 11.11   // Milliseconds per degree of turn (adjust after testing)

//=============================================================================
// TIME-BASED TURN FUNCTIONS (No odometry needed)
//=============================================================================

// Turn left - CORRECTED DIRECTION
double turn_left(oi_t *sensor_data, double degrees) {
    char buf[80];
    int duration_ms;

    // Calculate how many milliseconds to spin based on degrees
    duration_ms = (int)(degrees * MS_PER_DEGREE);

    sprintf(buf, "Turning LEFT %d degrees (%d ms)...\r\n", (int)degrees, duration_ms);
    uart_sendStr(buf);

    // To turn LEFT: Left wheel forward, Right wheel backward
    // This makes the robot pivot left
    oi_setWheels(150, -150);
    timer_waitMillis(duration_ms);
    oi_setWheels(0, 0);

    uart_sendStr("Left turn complete\r\n");
    timer_waitMillis(200);

    return degrees;
}

// Turn right - CORRECTED DIRECTION
double turn_right(oi_t *sensor_data, double degrees) {
    char buf[80];
    int duration_ms;

    // Calculate how many milliseconds to spin based on degrees
    duration_ms = (int)(degrees * MS_PER_DEGREE);

    sprintf(buf, "Turning RIGHT %d degrees (%d ms)...\r\n", (int)degrees, duration_ms);
    uart_sendStr(buf);

    // To turn RIGHT: Left wheel backward, Right wheel forward
    // This makes the robot pivot right
    oi_setWheels(-150, 150);
    timer_waitMillis(duration_ms);
    oi_setWheels(0, 0);

    uart_sendStr("Right turn complete\r\n");
    timer_waitMillis(200);

    return degrees;
}

// Drive forward for a specific distance (using odometry - this works)
double move_front(oi_t *sensor_data, double distance_mm) {
    double forward_distance = 0.0;
    double target = fabs(distance_mm);
    int bump_count = 0;

    oi_setWheels(200, 200);

    while (forward_distance < target) {
        oi_update(sensor_data);

        // Handle bumps
        if (sensor_data->bumpRight || sensor_data->bumpLeft) {
            bump_count++;
            oi_setWheels(0, 0);

            if (sensor_data->bumpRight) {
                double back = move_back(sensor_data, 150.0);
                forward_distance += back;
                turn_left(sensor_data, 90.0);
                move_front(sensor_data, 250.0);
                turn_right(sensor_data, 90.0);
            }
            else if (sensor_data->bumpLeft) {
                double back = move_back(sensor_data, 150.0);
                forward_distance += back;
                turn_right(sensor_data, 90.0);
                move_front(sensor_data, 250.0);
                turn_left(sensor_data, 90.0);
            }

            oi_setWheels(200, 200);
            timer_waitMillis(10);
            continue;
        }

        forward_distance += sensor_data->distance;
        timer_waitMillis(10);
    }

    oi_setWheels(0, 0);
    return forward_distance;
}

// Drive backward for a specific distance
double move_back(oi_t *sensor_data, double distance_mm) {
    double sum = 0.0;
    double target;

    if (distance_mm < 0) {
        target = -distance_mm;
    } else {
        target = distance_mm;
    }

    oi_setWheels(-200, -200);

    while (1) {
        oi_update(sensor_data);
        sum += sensor_data->distance;

        double abs_sum;
        if (sum < 0) {
            abs_sum = -sum;
        } else {
            abs_sum = sum;
        }

        if (abs_sum >= target) {
            break;
        }

        timer_waitMillis(10);
    }

    oi_setWheels(0, 0);
    return sum;
}
