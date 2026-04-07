/*
 * PingSensorPinScan.c
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

// Test PING sensor on a specific pin
float test_ping_on_pin(int port_base, int pin_mask, const char* pin_name) {
    volatile uint32_t *dirReg;
    volatile uint32_t *denReg;
    volatile uint32_t *dataReg;
    volatile uint32_t *lockReg = 0;
    volatile uint32_t *crReg = 0;
    int timeout;

    // Get register pointers for this port
    if(port_base == 0x40025000) { // Port F
        dirReg = &GPIO_PORTF_DIR_R;
        denReg = &GPIO_PORTF_DEN_R;
        dataReg = &GPIO_PORTF_DATA_R;
        lockReg = &GPIO_PORTF_LOCK_R;
        crReg = &GPIO_PORTF_CR_R;
    } else if(port_base == 0x40005000) { // Port B
        dirReg = &GPIO_PORTB_DIR_R;
        denReg = &GPIO_PORTB_DEN_R;
        dataReg = &GPIO_PORTB_DATA_R;
    } else if(port_base == 0x40006000) { // Port C
        dirReg = &GPIO_PORTC_DIR_R;
        denReg = &GPIO_PORTC_DEN_R;
        dataReg = &GPIO_PORTC_DATA_R;
    } else if(port_base == 0x40007000) { // Port D
        dirReg = &GPIO_PORTD_DIR_R;
        denReg = &GPIO_PORTD_DEN_R;
        dataReg = &GPIO_PORTD_DATA_R;
    } else if(port_base == 0x40024000) { // Port E
        dirReg = &GPIO_PORTE_DIR_R;
        denReg = &GPIO_PORTE_DEN_R;
        dataReg = &GPIO_PORTE_DATA_R;
    } else {
        return -1.0f;
    }

    // Unlock PF0 if needed
    if(port_base == 0x40025000 && pin_mask == 0x01) {
        *lockReg = 0x4C4F434B;
        *crReg |= pin_mask;
    }

    // Configure as output
    *dirReg |= pin_mask;
    *denReg |= pin_mask;

    // Send trigger pulse
    *dataReg &= ~pin_mask;  // Low
    timer_waitMicros(2);
    *dataReg |= pin_mask;   // High
    timer_waitMicros(5);
    *dataReg &= ~pin_mask;  // Low

    // Switch to input
    *dirReg &= ~pin_mask;
    timer_waitMicros(750);

    // Wait for echo
    timeout = 5000;
    while (!(*dataReg & pin_mask)) {
        timer_waitMicros(1);
        if (--timeout == 0) {
            *dirReg |= pin_mask;
            return -1.0f;
        }
    }

    // Measure echo pulse
    uint32_t echo_us = 0;
    timeout = 20000;
    while (*dataReg & pin_mask) {
        timer_waitMicros(1);
        echo_us++;
        if (--timeout == 0) {
            *dirReg |= pin_mask;
            return -1.0f;
        }
    }

    *dirReg |= pin_mask;
    return (float)echo_us / 58.0f;
}

int main(void) {
    char buffer[200];
    int i;
    float dist;

    uart_interrupt_init();
    timer_waitMillis(100);

    sendString("\r\n\r\n");
    sendString("========================================\r\n");
    sendString("PING SENSOR PIN SCANNER\r\n");
    sendString("========================================\r\n");
    sendString("\r\n");
    sendString("This will test ALL possible pins for the PING sensor.\r\n");
    sendString("Make sure the sensor is connected and facing an object.\r\n");
    sendString("You should hear clicking when the correct pin is tested.\r\n");
    sendString("\r\n");

    // Enable all GPIO clocks
    SYSCTL_RCGCGPIO_R |= 0x3F;  // Ports A-F
    timer_waitMillis(100);

    // Test Port C pins (where the code expects it)
    sendString("Testing Port C pins (expected on PC4)...\r\n");
    for(i = 0; i <= 7; i++) {
        int pin_mask = 1 << i;
        sprintf(buffer, "  PC%d: ", i);
        sendString(buffer);

        dist = test_ping_on_pin(0x40006000, pin_mask, "PC");
        sprintf(buffer, "%.1f cm\r\n", dist);
        sendString(buffer);

        if(dist > 2.0f && dist < 300.0f) {
            sprintf(buffer, "\r\n*** PING FOUND ON PC%d! Distance = %.1f cm ***\r\n", i, dist);
            sendString(buffer);
        }
        timer_waitMillis(200);
    }

    // Test Port B pins
    sendString("\r\nTesting Port B pins...\r\n");
    for(i = 0; i <= 7; i++) {
        int pin_mask = 1 << i;
        sprintf(buffer, "  PB%d: ", i);
        sendString(buffer);

        dist = test_ping_on_pin(0x40005000, pin_mask, "PB");
        sprintf(buffer, "%.1f cm\r\n", dist);
        sendString(buffer);

        if(dist > 2.0f && dist < 300.0f) {
            sprintf(buffer, "\r\n*** PING FOUND ON PB%d! Distance = %.1f cm ***\r\n", i, dist);
            sendString(buffer);
        }
        timer_waitMillis(200);
    }

    // Test Port D pins
    sendString("\r\nTesting Port D pins...\r\n");
    for(i = 0; i <= 7; i++) {
        int pin_mask = 1 << i;
        sprintf(buffer, "  PD%d: ", i);
        sendString(buffer);

        dist = test_ping_on_pin(0x40007000, pin_mask, "PD");
        sprintf(buffer, "%.1f cm\r\n", dist);
        sendString(buffer);

        if(dist > 2.0f && dist < 300.0f) {
            sprintf(buffer, "\r\n*** PING FOUND ON PD%d! Distance = %.1f cm ***\r\n", i, dist);
            sendString(buffer);
        }
        timer_waitMillis(200);
    }

    // Test Port E pins
    sendString("\r\nTesting Port E pins...\r\n");
    for(i = 0; i <= 5; i++) {
        int pin_mask = 1 << i;
        sprintf(buffer, "  PE%d: ", i);
        sendString(buffer);

        dist = test_ping_on_pin(0x40024000, pin_mask, "PE");
        sprintf(buffer, "%.1f cm\r\n", dist);
        sendString(buffer);

        if(dist > 2.0f && dist < 300.0f) {
            sprintf(buffer, "\r\n*** PING FOUND ON PE%d! Distance = %.1f cm ***\r\n", i, dist);
            sendString(buffer);
        }
        timer_waitMillis(200);
    }

    // Test Port F pins
    sendString("\r\nTesting Port F pins...\r\n");
    for(i = 0; i <= 4; i++) {
        int pin_mask = 1 << i;
        sprintf(buffer, "  PF%d: ", i);
        sendString(buffer);

        dist = test_ping_on_pin(0x40025000, pin_mask, "PF");
        sprintf(buffer, "%.1f cm\r\n", dist);
        sendString(buffer);

        if(dist > 2.0f && dist < 300.0f) {
            sprintf(buffer, "\r\n*** PING FOUND ON PF%d! Distance = %.1f cm ***\r\n", i, dist);
            sendString(buffer);
        }
        timer_waitMillis(200);
    }

    sendString("\r\n========================================\r\n");
    sendString("SCAN COMPLETE\r\n");
    sendString("========================================\r\n");
    sendString("\r\nIf no pin showed a distance reading:\r\n");
    sendString("1. The PING sensor might not be connected\r\n");
    sendString("2. The sensor might need 5V power (check connections)\r\n");
    sendString("3. The sensor might be damaged\r\n");
    sendString("\r\nIf you found a pin, update cyBot_Scan.c to use that pin!\r\n");

    while(1) {
        timer_waitMillis(100);
    }

    return 0;
}
