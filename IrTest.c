/*
 * IrTest.c
 *
 *  Created on: Mar 23, 2026
 *      Author: alecbroe
 */

#include "cyBot_Scan.h"
#include "open_interface.h"
#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include <stdio.h>

// ── UPDATE with your calibration values ────────────────────────────────────
#define MY_RIGHT_CAL      500
#define MY_LEFT_CAL       2100

// Function to read from a specific ADC channel
int read_adc_channel(int channel) {
    // Disable SS3
    ADC0_ACTSS_R &= ~0x08;
    // Set the channel
    ADC0_SSMUX3_R = channel;
    // Configure SS3 for single sample
    ADC0_SSCTL3_R = 0x06;
    // Re-enable SS3
    ADC0_ACTSS_R |= 0x08;

    // Start conversion
    ADC0_PSSI_R = 0x08;
    // Wait for conversion to complete
    while ((ADC0_RIS_R & 0x08) == 0);
    // Read result
    int value = ADC0_SSFIFO3_R & 0xFFF;
    // Clear interrupt
    ADC0_ISC_R = 0x08;

    return value;
}

void configure_adc_pins(void) {
    // Enable ADC0 clock
    SYSCTL_RCGCADC_R |= 0x01;
    timer_waitMillis(5);

    // Enable GPIO clocks
    SYSCTL_RCGCGPIO_R |= 0x08;  // Port D
    SYSCTL_RCGCGPIO_R |= 0x10;  // Port E
    SYSCTL_RCGCGPIO_R |= 0x02;  // Port B
    timer_waitMillis(5);

    // Configure Port E (pins 0-3 for AIN0-3)
    GPIO_PORTE_AMSEL_R |= 0x0F;
    GPIO_PORTE_DEN_R &= ~0x0F;
    GPIO_PORTE_AFSEL_R &= ~0x0F;

    // Configure Port D (pins 0-3 for AIN4-7)
    GPIO_PORTD_AMSEL_R |= 0x0F;
    GPIO_PORTD_DEN_R &= ~0x0F;
    GPIO_PORTD_AFSEL_R &= ~0x0F;

    // Configure Port E (pins 4-5 for AIN8-9)
    GPIO_PORTE_AMSEL_R |= 0x30;
    GPIO_PORTE_DEN_R &= ~0x30;
    GPIO_PORTE_AFSEL_R &= ~0x30;

    // Configure Port B (pins 4-5 for AIN10-11)
    GPIO_PORTB_AMSEL_R |= 0x30;
    GPIO_PORTB_DEN_R &= ~0x30;
    GPIO_PORTB_AFSEL_R &= ~0x30;

    // Configure ADC0 SS3
    ADC0_ACTSS_R &= ~0x08;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSCTL3_R = 0x06;
    ADC0_ACTSS_R |= 0x08;
}

int main(void) {
    char buf[100];
    int channel_values[12];
    int i, j;

    timer_init();
    oi_t *sensor = oi_alloc();
    oi_init(sensor);
    uart_interrupt_init();
    lcd_init();

    configure_adc_pins();

    uart_sendStr("\r\n========================================\r\n");
    uart_sendStr("  IR SENSOR DIAGNOSTIC TEST\r\n");
    uart_sendStr("========================================\r\n");
    uart_sendStr("\r\nPut your hand in front of the IR sensor\r\n");
    uart_sendStr("Press 's' to start scanning...\r\n");
    lcd_printf("IR Diag\nPress s");

    while (1) {
        if (byte_received == 's') {
            byte_received = '\0';

            uart_sendStr("\r\n\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("Baseline Readings (no hand)\r\n");
            uart_sendStr("========================================\r\n\n");
            uart_sendStr("Channel  Pin    Value\r\n");
            uart_sendStr("-------  ----   -----\r\n");

            // First reading - baseline
            for (i = 0; i < 12; i++) {
                channel_values[i] = read_adc_channel(i);
                sprintf(buf, "AIN%d     ", i);
                uart_sendStr(buf);
                // Add pin numbers based on channel
                if (i == 0) uart_sendStr("PE3   ");
                else if (i == 1) uart_sendStr("PE2   ");
                else if (i == 2) uart_sendStr("PE1   ");
                else if (i == 3) uart_sendStr("PE0   ");
                else if (i == 4) uart_sendStr("PD3   ");
                else if (i == 5) uart_sendStr("PD2   ");
                else if (i == 6) uart_sendStr("PD1   ");
                else if (i == 7) uart_sendStr("PD0   ");
                else if (i == 8) uart_sendStr("PE5   ");
                else if (i == 9) uart_sendStr("PE4   ");
                else if (i == 10) uart_sendStr("PB4   ");
                else if (i == 11) uart_sendStr("PB5   ");
                sprintf(buf, " %d\r\n", channel_values[i]);
                uart_sendStr(buf);
                timer_waitMillis(100);
            }

            uart_sendStr("\r\nPress 's' AGAIN when you have your hand in front...\r\n");
            lcd_printf("Place hand\nPress s");

            // Wait for second press
            while (byte_received != 's') {
                // Wait
            }
            byte_received = '\0';

            uart_sendStr("\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("Readings WITH HAND IN FRONT\r\n");
            uart_sendStr("========================================\r\n\n");
            uart_sendStr("Channel  Pin    Value   Change\r\n");
            uart_sendStr("-------  ----   -----   ------\r\n");

            // Second reading - with hand
            for (i = 0; i < 12; i++) {
                int new_value = read_adc_channel(i);
                int delta = channel_values[i] - new_value;
                if (delta < 0) delta = -delta;

                sprintf(buf, "AIN%d     ", i);
                uart_sendStr(buf);
                // Add pin numbers
                if (i == 0) uart_sendStr("PE3   ");
                else if (i == 1) uart_sendStr("PE2   ");
                else if (i == 2) uart_sendStr("PE1   ");
                else if (i == 3) uart_sendStr("PE0   ");
                else if (i == 4) uart_sendStr("PD3   ");
                else if (i == 5) uart_sendStr("PD2   ");
                else if (i == 6) uart_sendStr("PD1   ");
                else if (i == 7) uart_sendStr("PD0   ");
                else if (i == 8) uart_sendStr("PE5   ");
                else if (i == 9) uart_sendStr("PE4   ");
                else if (i == 10) uart_sendStr("PB4   ");
                else if (i == 11) uart_sendStr("PB5   ");
                sprintf(buf, " %d", new_value);
                uart_sendStr(buf);
                sprintf(buf, "      %d\r\n", delta);
                uart_sendStr(buf);

                // Update LCD with the channel that changed most
                if (delta > 50) {
                    lcd_printf("AIN%d\nPin %s\nDelta %d", i,
                              (i==0?"PE3":i==1?"PE2":i==2?"PE1":i==3?"PE0":
                               i==4?"PD3":i==5?"PD2":i==6?"PD1":i==7?"PD0":
                               i==8?"PE5":i==9?"PE4":i==10?"PB4":"PB5"), delta);
                }
                timer_waitMillis(100);
            }

            uart_sendStr("\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("Which channel had the BIGGEST change?\r\n");
            uart_sendStr("That's your IR sensor!\r\n");
            uart_sendStr("========================================\r\n");
            uart_sendStr("\r\nPress 's' to test again.\r\n");
            lcd_printf("Done!\nPress s");
        }
    }

    oi_free(sensor);
    return 0;
}
