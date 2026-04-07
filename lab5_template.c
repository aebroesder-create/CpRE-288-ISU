/**
 * lab5_template.c
 *
 * Template file for CprE 288 Lab 5
 *
 * @author Zhao Zhang, Chad Nelson, Zachary Glanz
 * @date 08/14/2016
 *
 * @author Phillip Jones, updated 6/4/2019
 * @author Diane Rover, updated 2/25/2021, 2/17/2022
 */

/*
 * lab5_part1.c - Part 1
 */
#include "button.h"
#include "timer.h"
#include "lcd.h"
#include "cyBot_uart.h"
#include "cyBot_Scan.h"
#include <stdint.h>

/* Register addresses for GPIO Port B init */
#define SYSCTL_RCGCGPIO   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO     (*((volatile uint32_t *)0x400FEA08))
#define GPIOB_AFSEL       (*((volatile uint32_t *)0x40005420))
#define GPIOB_DEN         (*((volatile uint32_t *)0x4000551C))
#define GPIOB_PCTL        (*((volatile uint32_t *)0x4000552C))

int main(void) {
    button_init();
    timer_init();
    lcd_init();

    /* Reset UART1 to a clean state before our init code */
    cyBot_uart_init_clean();

    /* --- YOUR GPIO INIT CODE (first half) --- */

    /* 1. Enable clock for GPIO Port B */
    SYSCTL_RCGCGPIO |= 0x02;

    /* 2. Wait for Port B to be ready */
    while ((SYSCTL_PRGPIO & 0x02) == 0) {}

    /* 3. Digital enable on PB0 (U1RX) and PB1 (U1TX) */
    GPIOB_DEN |= 0x03;

    /* 4. Alternate function on PB0 and PB1 */
    GPIOB_AFSEL |= 0x03;

    /* 5. UART1 (PMC=1) assigned to PB0 and PB1 */
    GPIOB_PCTL &= 0xFFFFFF00;
    GPIOB_PCTL |= 0x00000011;

    /* --- Library completes UART device configuration --- */
    cyBot_uart_init_last_half();

    /* Test: send a message to PuTTY on startup */
    lcd_printf("Part 1 Ready");
    cyBot_sendByte('H');
    cyBot_sendByte('e');
    cyBot_sendByte('l');
    cyBot_sendByte('l');
    cyBot_sendByte('o');
    cyBot_sendByte('\r');
    cyBot_sendByte('\n');

    while (1) {
        /* Receive a character from PuTTY - NOTE: correct function name is
         * cyBot_getByte_blocking() in the Lab 5 library */
        char c = cyBot_getByte_blocking();

        /* Echo it back to PuTTY */
        cyBot_sendByte(c);

        /* Show on LCD */
        lcd_printf("Got: %c", c);
    }

    return 0;
}
