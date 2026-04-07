/*
 * PinScanner.c
 *
 *  Created on: Mar 23, 2026
 *      Author: alecbroe
 */


#include "Timer.h"
#include "lcd.h"
#include "uart-interrupt.h"
#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include <stdio.h>

static char buf[256];
static void print(const char *s)   { uart_sendStr(s); }
static void println(const char *s) { uart_sendStr(s); uart_sendStr("\r\n"); }

static void wait_for_s(void) {
    println("  >> Press 's' to continue...");
    while (byte_received != 's') { timer_waitMillis(50); }
    byte_received = 0;
}

// Channel name lookup
static const char *ch_name(int ch) {
    switch(ch) {
        case  0: return "AIN0 PE3";
        case  1: return "AIN1 PE2";
        case  2: return "AIN2 PE1";
        case  3: return "AIN3 PE0";
        case  4: return "AIN4 PD3";
        case  5: return "AIN5 PD2";
        case  6: return "AIN6 PD1";
        case  7: return "AIN7 PD0";
        case  8: return "AIN8 PE5";
        case  9: return "AIN9 PE4";
        case 10: return "AIN10 PB4";
        case 11: return "AIN11 PB5";
        default: return "???";
    }
}

// Enable every possible analog pin
static void enable_all_analog(void) {
    SYSCTL_RCGCGPIO_R |= 0x1A;  // ports B, D, E
    while ((SYSCTL_PRGPIO_R & 0x1A) != 0x1A);

    // PE0-PE5
    GPIO_PORTE_DEN_R   &= ~0x3F;
    GPIO_PORTE_AFSEL_R &= ~0x3F;
    GPIO_PORTE_AMSEL_R |=  0x3F;

    // PD0-PD3
    GPIO_PORTD_DEN_R   &= ~0x0F;
    GPIO_PORTD_AFSEL_R &= ~0x0F;
    GPIO_PORTD_AMSEL_R |=  0x0F;

    // PB4-PB5
    GPIO_PORTB_DEN_R   &= ~0x30;
    GPIO_PORTB_AFSEL_R &= ~0x30;
    GPIO_PORTB_AMSEL_R |=  0x30;

    SYSCTL_RCGCADC_R |= 0x01;
    timer_waitMillis(10);
}

// Read a single ADC channel
static int adc_read(int ch) {
    ADC0_ACTSS_R  &= ~0x08;
    ADC0_EMUX_R   &= ~0xF000;
    ADC0_SSMUX3_R  = (uint32_t)ch;
    ADC0_SSCTL3_R  = 0x06;
    ADC0_ACTSS_R  |=  0x08;
    ADC0_PSSI_R    = 0x08;
    int t = 100000;
    while ((ADC0_RIS_R & 0x08) == 0) { if (--t == 0) return -1; }
    int v = ADC0_SSFIFO3_R & 0xFFF;
    ADC0_ISC_R = 0x08;
    return v;
}

// Read each channel multiple times and return average
static int adc_avg(int ch, int n) {
    long sum = 0; int i;
    for (i = 0; i < n; i++) {
        sum += adc_read(ch);
        timer_waitMillis(2);
    }
    return (int)(sum / n);
}

int main(void) {
    timer_init();
    lcd_init();
    uart_interrupt_init();
    enable_all_analog();

    lcd_printf("Pin\nScanner");

    println("\r\n============================================");
    println("  CyBot IR Pin Scanner");
    println("============================================");
    println("  This will show ALL 12 ADC channels live.");
    println("  Wave your hand in front of the IR sensor.");
    println("  The channel that CHANGES is your IR pin.");
    println("  Press 's' to stop the live feed.");
    println("============================================");
    wait_for_s();

    // ── Step 1: Baseline — read all channels with nothing in front ──────────
    println("\r\n--- BASELINE (nothing in front of IR sensor) ---");
    int baseline[12];
    int ch;
    for (ch = 0; ch < 12; ch++) {
        baseline[ch] = adc_avg(ch, 10);
    }
    println("Ch    Pin      Baseline");
    println("--    ---      --------");
    for (ch = 0; ch < 12; ch++) {
        sprintf(buf, "%2d    %-9s  %4d\r\n", ch, ch_name(ch), baseline[ch]);
        print(buf);
    }

    // ── Step 2: Live feed — print all channels, highlight changes ───────────
    println("\r\n--- LIVE FEED (wave hand in front of sensor, press 's' to stop) ---");
    println("Watching for channels that differ from baseline by >100...\r\n");

    timer_waitMillis(1000);
    byte_received = 0;

    int scan_num = 0;
    while (byte_received != 's') {
        scan_num++;
        sprintf(buf, "--- Scan %d ---\r\n", scan_num);
        print(buf);

        for (ch = 0; ch < 12; ch++) {
            int v = adc_avg(ch, 5);
            int delta = v - baseline[ch];
            // Mark channels that changed significantly
            const char *flag = (delta > 100 || delta < -100) ? " <--- CHANGED!" : "";
            sprintf(buf, "  Ch%2d %-9s  %4d  (delta %+5d)%s\r\n",
                    ch, ch_name(ch), v, delta, flag);
            print(buf);
        }
        println("");
        timer_waitMillis(500);
    }
    byte_received = 0;

    // ── Step 3: Summary ─────────────────────────────────────────────────────
    println("\r\n============================================");
    println("  SCAN STOPPED.");
    println("  Look above for lines marked <--- CHANGED!");
    println("  That channel number is your IR ADC channel.");
    println("  Tell Claude the channel number (0-11) and");
    println("  we will hardcode it in cyBot_Scan.c.");
    println("============================================");

    lcd_printf("Check\nPuTTY");
    return 0;
}
