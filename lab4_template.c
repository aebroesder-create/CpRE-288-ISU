/**
 * lab4_template.c
 *
 * Template file for CprE 288 lab 4
 *
 * @author Zhao Zhang, Chad Nelson, Zachary Glanz
 * @date 08/14/2016
 */


#include "button.h"
#include "Timer.h"
#include "lcd.h"
#include "open_interface.h"
#include "cyBot_Scan.h"
#include "cyBot_uart.h"// Functions for communicating between CyBot and Putty (via UART)
                         // PuTTy: Baud=115200, 8 data bits, No Flow Control, No Parity, COM1


#define REPLACEME 0

void sendString(const char* str) { //Helps send string via UART
    while (*str != '\0') {
        cyBot_sendByte(*str);
        str++;
    }
}

int main(void) {
    button_init();
    timer_init(); // Must be called before lcd_init(), which uses timer functions
    lcd_init();
    cyBot_uart_init();

                // Don't forget to initialize the cyBot UART before trying to use it
    sendString("UART Initialized. Waiting for input...\r\n");
        // Infinite loop for polling

    uint8_t previous_btn = 0; //Tracks the last button state to prevent spam
    while(1)
        {
            // Read the button status
            // Returns 1-4 if pressed, or 0 if released
            uint8_t btn = button_getButton();

            if(btn !=0 && btn != previous_btn) { //Prevents spam

            // Update LCD only if a button is actively pressed
            if (btn != 0) {
                lcd_printf("Button: %d", btn);
                switch(btn) {
                  case 1:
                  sendString("Button 1 Pressed\r\n");
                break;
                case 2:
                  sendString("Button 2 Pressed\r\n");
                  break;
                  case 3:
                   sendString("Button 3 Pressed\r\n");
                      break;
                  case 4:
                        sendString("Button 4 Pressed\r\n");
                           break;
               default:
                    sendString("Press a button\r\n");
                    break;
                   }
            }

previous_btn = btn;
         }
    }
}
