/*
 * Lab5PT4.c
 *
 *  Created on: Mar 2, 2026
 *      Author: alecbroe
 */

#include "button.h"
#include "timer.h"
#include "lcd.h"
#include "L5UART.h"
#include "cyBot_Scan.h"

#define MAX 20  // Maximum buffer size for characters

int main(void) {
    // Initialize all peripherals
    button_init();      // Initialize buttons (not used but good practice)
    timer_init();       // Initialize timer for any timing needs
    lcd_init();         // Initialize LCD display
    uart_init();        // Initialize UART1 for CyBot-PC communication (115200 baud)

    /* --- Startup banner sent to PuTTY ---
     * This is PART 4 functionality - CyBot initiates its own messages
     * The \r\n creates proper newlines in PuTTY
     */
    uart_sendStr("========================================\r\n");
    uart_sendStr("  CprE 288 Lab 5 - Part 4\r\n");
    uart_sendStr("  CyBot <-> PuTTY Communication Demo\r\n");
    uart_sendStr("  Type up to 20 chars or press ENTER\r\n");
    uart_sendStr("  to display on LCD.\r\n");
    uart_sendStr("========================================\r\n\r\n");

    lcd_printf("Part 4 Ready");  // Show ready message on LCD

    char buf[MAX + 1];    // Buffer to store received characters (+1 for null terminator)
    int count = 0;         // Current number of characters in buffer

    // Main program loop - continuously runs
    while (1) {
        // BLOCKING CALL: Waits here until a character is received from PuTTY
        // This is the receive functionality from Part 2/3
        char c = uart_receive();

        // CASE 1: ENTER key pressed (Carriage Return)
        if (c == '\r') {
            /* ENTER Handling - Part 3 functionality with Part 4 enhancements */

            // Send CR+LF to move to new line in PuTTY (proper line ending)
            uart_sendChar('\r');
            uart_sendChar('\n');

            // Null-terminate the buffer to make it a proper string
            buf[count] = '\0';

            /* PART 4: CyBot initiates a message to PuTTY */
            uart_sendStr(">> LCD now shows: \"");
            uart_sendStr(buf);           // Send the buffered string
            uart_sendStr("\"\r\n");       // Close quotes and newline

            // Update LCD with the complete string (Part 3 functionality)
            lcd_clear();                  // Clear LCD before new display
            lcd_printf("%s", buf);         // Show the complete string

            // Reset buffer for next input
            count = 0;

        // CASE 2: Buffer full (reached MAX characters)
        } else if (count >= MAX) {
            /* Buffer Full Handling - similar to ENTER but with different message */

            // Echo the character back to PuTTY (Part 3)
            uart_sendChar(c);

            // Store the last character and null-terminate
            buf[MAX - 1] = c;
            buf[MAX] = '\0';

            /* PART 4: CyBot initiates a message to PuTTY */
            uart_sendStr("\r\n>> Buffer full! LCD now shows: \"");
            uart_sendStr(buf);
            uart_sendStr("\"\r\n");

            // Update LCD (Part 3)
            lcd_clear();
            lcd_printf("%s", buf);

            // Reset buffer
            count = 0;

        // CASE 3: Normal character (not ENTER, buffer not full)
        } else {
            /* Normal Character Handling */

            // Echo the character back to PuTTY (Part 3 - basic echo functionality)
            uart_sendChar(c);

            // Store character in buffer (Part 2 - buffer management)
            buf[count] = c;
            count++;
            buf[count] = '\0';  // Keep buffer null-terminated for string functions


            // Update LCD with current buffer contents and count (Part 3)
            // The %s shows the string, \n moves to line 2, %d shows the count
            lcd_clear();
            lcd_printf("%s\n%d", buf, count);
        }
    }

    return 0;  // Never reached, but good practice
}
