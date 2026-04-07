
#include "Timer.h"
#include "lcd.h"

int main(){

    timer_init();
    lcd_init();

    int LCD_WIDTH = 20;
    const char message[] = "Microcontrollers are lots of fun!";
            char line[LCD_WIDTH + 1];
            int msgLen = strlen(message);

            int pos;
            int i;
            int msgIndex;

            while (1)
            {
                for (pos = 0; pos < msgLen + LCD_WIDTH; pos++)
                {
                    for (i = 0; i < LCD_WIDTH; i++)
                    {
                        msgIndex = pos - (LCD_WIDTH - 1) + i;

                        if (msgIndex >= 0 && msgIndex < msgLen)
                            line[i] = message[msgIndex];
                        else
                            line[i] = ' ';
                    }

                    line[LCD_WIDTH] = '\0';

                    lcd_printf("%s", line);
                    timer_waitMillis(300);
                }

                /* Screen cleared for 0.3 seconds */
                lcd_printf("                     "); // 20 spaces
                timer_waitMillis(300);
            }
}
