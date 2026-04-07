/*
 * main.c
 *
 *  Created on: Feb 3, 2026
 *      Author: alecbroe
 */


#include "movement.h"
#include "open_interface.h"
#include "timer.h"
#include "lcd.h"

int main() {
    timer_init();

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);


    move_front(sensor_data, 2000.0);

    while(1);

    return 0;
}
