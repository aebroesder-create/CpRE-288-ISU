/*
 * PingEchoTest.c
 *
 *  Created on: Mar 27, 2026
 *      Author: alecbroe
 */

#include "cyBot_Scan.h"
#include "uart-interrupt.h"
#include "Timer.h"
#include <stdio.h>
#include <string.h>

void sendString(char *str) {
    int i;
    for (i = 0; i < strlen(str); i++) {
        uart_sendChar(str[i]);
    }
}

int main(void) {
    char buffer[200];
    cyBOT_Scan_t scan;

    uart_interrupt_init();
    timer_waitMillis(100);
    cyBOT_init_Scan(0b0011);  // Enable servo and PING

    // Your calibration values
    right_calibration_value = 311500;
    left_calibration_value = 1309000;

    sendString("\r\n\r\n");
    sendString("========================================\r\n");
    sendString("TESTING UPDATED PING SENSOR\r\n");
    sendString("========================================\r\n");
    sendString("\r\n");
    sendString("The PING sensor should now be on PB3\r\n");
    sendString("\r\n");

    while(1) {
        cyBOT_Scan(90, &scan);
        sprintf(buffer, "Distance: %.1f cm | IR: %d\r\n", scan.sound_dist, scan.IR_raw_val);
        sendString(buffer);
        timer_waitMillis(500);
    }

    return 0;
}
