/*
 * main.c
 *
 *  Created on: Feb 17, 2026
 *      Author: alecbroe
 */

#include "Timer.h"
#include "lcd.h"
#include "open_interface.h"
#include "movement.h"
#include "cyBot_uart.h"
#include "cyBot_Scan.h"
#include <string.h>
#include "button.h"

#define TOTAL_DEG_TURN 180
#define DEGREE_INCREMENT 2

int lcd_rotatingBanner(int i);

void sendString(char c, char puttyString[]);

void printString(char c);

void printWholeString(char puttyString[]);

int main (void) {

    oi_t *sensor_data = oi_alloc();

    oi_init(sensor_data);

    button_init(); //initializes button GPIOs, found in button.c

    timer_init(); // Initialize Timer, needed before any LCD screen functions can be called
                  // and enables time functions (e.g. timer_waitMillis)
    lcd_init();   // Initialize the LCD screen and also clears the screen.

    cyBot_uart_init();

    char puttyString[50];
    int button;
    for(;;){
        button = button_getButton();
        sprintf(puttyString, "%i\n", button);
        lcd_home();
        lcd_putc(puttyString[0]);
        printWholeString(puttyString);

    }






    oi_free(sensor_data);
    return 0;
}


void sendString(char c, char puttyString[]){ //adds char to end of string

    sprintf(puttyString, "%s%c", puttyString, c);
    lcd_printf("Got %s\n", puttyString);

}

void printWholeString(char puttyString[]){ //prints to putty
    int i;
    for(i=0;i<strlen(puttyString);i++){
        printString(puttyString[i]);
    }

}

void printString(char c){ //alternative to sendByte
    cyBot_sendByte(c);
}

int lcd_rotatingBanner(int i){ //ignore for this lab, creates rotating banner display

    char str[] = "                    ;)                    ";



    int j;
    for(j=0;j<20;j++){

        lcd_putc(str[j+i]);
    }



    i++;

    if(str[j+i+1]=='\0'){
        i=0;
    }


return i;
}
