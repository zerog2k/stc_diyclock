//
// STC15F204EA DIY LED Clock
// Copyright 2016, Jens Jensen
//

#include "stc15.h"
#include <stdint.h>
#include <stdio.h>
#include "adc.h"
#include "ds1302.h"
#include "led.h"
    
#define FOSC    11059200

// clear wdt
#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4)

// alias for relay and buzzer outputs, using relay to drive led for indication of main loop status
// only for revision with stc15f204ea
#if defined stc15f204ea || defined stc15w404as
 #define RELAY   P1_4
 #define BUZZER  P1_5
 // additional pins on P3 header: P3_6 P3_7
#else // revision with stc15w408as (with voice chip)
 #define LED     P1_5
#endif

// adc channels for sensors
#define ADC_LIGHT 6
#define ADC_TEMP  7

// button switch aliases
// SW3 only for revision with stc15w408as
#ifdef stc15w408as
 #define SW3     P1_4
 #define NUM_SW 3
#else
 #define NUM_SW 2
#endif
#define SW2     P3_0
#define SW1     P3_1

// display mode states
enum keyboard_mode {
    K_NORMAL,
    K_SET_HOUR,
    K_SET_MINUTE,
    K_SET_HOUR_12_24,
    K_SEC_DISP,
    K_TEMP_DISP,
#ifndef WITHOUT_DATE
    K_DATE_DISP,
    K_SET_MONTH,
    K_SET_DAY,
#endif
    K_WEEKDAY_DISP,
#ifdef DEBUG
    K_DEBUG,
    K_DEBUG2,
    K_DEBUG3,
#endif
};

// display mode states
enum display_mode {
    M_NORMAL,
    M_SET_HOUR_12_24,
    M_SEC_DISP,
    M_TEMP_DISP,
#ifndef WITHOUT_DATE
    M_DATE_DISP,
#endif
    M_WEEKDAY_DISP,
#ifdef DEBUG
    M_DEBUG,
    M_DEBUG2,
    M_DEBUG3,
#endif
};
#define NUM_DEBUG 3

/* ------------------------------------------------------------------------- */
/*
void _delay_ms(uint8_t ms)
{
    // delay function, tuned for 11.092 MHz clock
    // optimized to assembler
    ms; // keep compiler from complaining?
    __asm;
        ; dpl contains ms param value
    delay$:
        mov	b, #8   ; i
    outer$:
        mov	a, #243    ; j
    inner$:
        djnz acc, inner$
        djnz b, outer$
        djnz dpl, delay$
    __endasm;
}
*/

uint8_t  count;     // main loop counter
uint16_t temp;      // temperature sensor value
uint8_t  lightval;  // light sensor value

volatile uint8_t displaycounter;
volatile int8_t count_100;
volatile int16_t count_1000;
volatile int16_t count_5000;
volatile __bit loop_gate;
volatile __bit blinker_slow;
volatile __bit blinker_fast;

uint8_t dmode = M_NORMAL;     // display mode state
uint8_t kmode = K_NORMAL;

__bit  flash_01;
__bit  flash_23;

volatile __bit S1_LONG;
volatile __bit S1_PRESSED;
volatile __bit S2_LONG;
volatile __bit S2_PRESSED;
#ifdef stc15w408as
volatile __bit S3_LONG;
volatile __bit S3_PRESSED;
#endif

volatile uint8_t debounce[NUM_SW];      // switch debounce buffer
volatile uint8_t switchcount[NUM_SW];
#define SW_CNTMAX 80

enum Event {
    EV_NONE,
    EV_S1_SHORT,
    EV_S1_LONG,
    EV_S2_SHORT,
    EV_S2_LONG,
    EV_S1S2_LONG,
#ifdef stc15w408as
    EV_S3_SHORT,
    EV_S3_LONG,
#endif
};

volatile enum Event event;

void timer0_isr() __interrupt 1 __using 1
{
    enum Event ev = EV_NONE;
    // display refresh ISR
    // cycle thru digits one at a time
    uint8_t digit = displaycounter % 4;

    // turn off all digits, set high
    P3 |= 0x3C;

    // auto dimming, skip lighting for some cycles
    if (displaycounter % lightval < 4 ) {
        // fill digits
        P2 = dbuf[digit];
        // turn on selected digit, set low
        P3 &= ~(0x4 << digit);
    }
    displaycounter++;

    // 100/sec: 10 ms
    if (count_100 == 100) {
        count_100 = 0;
        // 10/sec: 100 ms
        if (count_1000 == 1000) {
            count_1000 = 0;
            blinker_fast = !blinker_fast;
            loop_gate = 1;
            // 2/sec: 500 ms
            if (count_5000 == 5000) {
                count_5000 = 0;
                blinker_slow = !blinker_slow;
            }
        }

#define MONITOR_S(n) \
        { \
            uint8_t s = n - 1; \
            /* read switch positions into sliding 8-bit window */ \
            debounce[s] = (debounce[s] << 1) | SW ## n ; \
            if (debounce[s] == 0) { \
                /* down for at least 8 ticks */ \
                S ## n ## _PRESSED = 1; \
                if (!S ## n ## _LONG) { \
                    switchcount[s]++; \
                } \
            } else { \
                /* released or bounced */ \
                if (S ## n ## _PRESSED) { \
                    if (!S ## n ## _LONG) { \
                        ev = EV_S ## n ## _SHORT; \
                    } \
                    S ## n ## _PRESSED = 0; \
                    S ## n ## _LONG = 0; \
                    switchcount[s] = 0; \
                } \
            } \
            if (switchcount[s] > SW_CNTMAX) { \
                S ## n ## _LONG = 1; \
                switchcount[s] = 0; \
                ev = EV_S ## n ## _LONG; \
            } \
        }

        MONITOR_S(1);
        MONITOR_S(2);
#ifdef stc15w408as
        MONITOR_S(3);
#endif

        if (ev == EV_S1_LONG && S2_PRESSED) {
            S2_LONG = 1;
            switchcount[1] = 0;
            ev = EV_S1S2_LONG;
        } else if (ev == EV_S2_LONG && S1_PRESSED) {
            S1_LONG = 1;
            switchcount[0] = 0;
            ev = EV_S1S2_LONG;
        }
        if (event == EV_NONE) {
            event = ev;
        }
    }
    count_100++;
    count_1000++;
    count_5000++;
}

/*
// macro expansion for MONITOR_S(1)
{
    uint8_t s = 1 - 1;
    debounce[s] = (debounce[s] << 1) | SW1 ;
    if (debounce[s] == 0) {
        S_PRESSED = 1;
        if (!S_LONG) {
            switchcount[s]++;
        }
    } else {
        if (S1_PRESSED) {
            if (!S1_LONG) {
                ev = EV_S1_SHORT;
            }
            S1_PRESSED = 0;
            S1_LONG = 0;
            switchcount[s] = 0;
        }
    }
    if (switchcount[s] > SW_CNTMAX) {
        S1_LONG = 1;
        switchcount[s] = 0;
        ev = EV_S1_LONG;
    }
}
*/

// Call timer0_isr() 10000/sec: 0.0001 sec
// Initialize the timer count so that it overflows after 0.0001 sec
// THTL = 0x10000 - FOSC / 12 / 10000 = 0x10000 - 92.16 = 65444 = 0xFFA4
void Timer0Init(void)		//100us @ 11.0592MHz
{
    // refer to section 7 of datasheet: STC15F2K60S2-en2.pdf
    // TMOD = 0;    // default: 16-bit auto-reload
    // AUXR = 0;    // default: traditional 8051 timer frequency of FOSC / 12
    // Initial values of TL0 and TH0 are stored in hidden reload registers: RL_TL0 and RL_TH0
	TL0 = 0xA4;		// Initial timer value
	TH0 = 0xFF;		// Initial timer value
    TF0 = 0;		// Clear overflow flag
    TR0 = 1;		// Timer0 start run
    ET0 = 1;        // Enable timer0 interrupt
    EA = 1;         // Enable global interrupt
}

int8_t gettemp(uint16_t raw) {
    // formula for ntc adc value to approx C
    float tempC = 76. - raw * 64. / 637.;
    // convert to F if configured
    return (CONF_C_F ? tempC * 1.8 + 32 : tempC)
        + (cfg_table[CFG_TEMP_BYTE] & CFG_TEMP_MASK) - 4;
}

/*********************************************/
int main()
{
    // SETUP
    // set photoresistor & ntc pins to open-drain output
    P1M1 |= (1<<6) | (1<<7);
    P1M0 |= (1<<6) | (1<<7);

    // init rtc
    ds_init();
    // init/read ram config
    ds_ram_config_init();

    // uncomment in order to reset minutes and hours to zero.. Should not need this.
    //ds_reset_clock();

    Timer0Init(); // display refresh & switch read

    // LOOP
    while (1)
    {
        enum Event ev;

        while (!loop_gate); // wait for open
        loop_gate = 0; // close gate

        ev = event;
        event = EV_NONE;

//#ifdef RELAY
//        RELAY = blinker_slow;
//#endif
//#ifdef LED
//        LED = blinker_slow;
//#endif
//#ifdef BUZZER
//        BUZZER = blinker_slow;
//#endif

        // sample adc, run frequently
        if (count % 4 == 0) {
            temp = gettemp(getADCResult(ADC_TEMP));
            // auto-dimming, by dividing adc range into 8 steps
            lightval = getADCResult8(ADC_LIGHT) >> 3;
            // set floor of dimming range
            if (lightval < 4) {
                lightval = 4;
            }
        }

        ds_readburst(); // read rtc

        // keyboard decision tree
        switch (kmode) {

            case K_SET_HOUR:
                flash_01 = 1;
                if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast)) {
                    ds_hours_incr();
                }
                else if (ev == EV_S1_SHORT) {
                    kmode = K_SET_MINUTE;
                }
                break;

            case K_SET_MINUTE:
                flash_01 = 0;
                flash_23 = 1;
                if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast)) {
                    ds_minutes_incr();
                }
                else if (ev == EV_S1_SHORT) {
                    kmode = K_SET_HOUR_12_24;
                }
                break;

            case K_SET_HOUR_12_24:
                dmode = M_SET_HOUR_12_24;
                if (ev == EV_S2_SHORT) {
                    ds_hours_12_24_toggle();
                }
                else if (ev == EV_S1_SHORT) {
                    kmode = K_NORMAL;
                }
                break;

            case K_TEMP_DISP:
                dmode = M_TEMP_DISP;
                if (ev == EV_S1_SHORT) {
                    // increment offset
                    uint8_t offset = cfg_table[CFG_TEMP_BYTE] & CFG_TEMP_MASK;
                    offset++;
                    offset &= CFG_TEMP_MASK;
                    cfg_table[CFG_TEMP_BYTE] = (cfg_table[CFG_TEMP_BYTE] & ~CFG_TEMP_MASK) | offset;
                    ds_ram_config_write();
                }
                else if (ev == EV_S1_LONG) {
                    // toggle C/F
                    CONF_C_F = !CONF_C_F;
                    ds_ram_config_write();
                }
                else if (ev == EV_S2_SHORT) {
#ifndef WITHOUT_DATE
                    kmode = K_DATE_DISP;
#else
                    kmode = K_WEEKDAY_DISP;
#endif
                }
                break;

#ifndef WITHOUT_DATE
            case K_DATE_DISP:
                dmode = M_DATE_DISP;
                if (ev == EV_S1_SHORT) {
                    // toggle MMDD
                    CONF_SW_MMDD = !CONF_SW_MMDD;
                    ds_ram_config_write();
                }
                else if (ev == EV_S1_LONG) {
                    kmode = CONF_SW_MMDD ? K_SET_DAY : K_SET_MONTH;
                }
                else if (ev == EV_S2_SHORT) {
                    kmode = K_WEEKDAY_DISP;
                }
                break;

            case K_SET_MONTH:
                flash_01 = 1;
                if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast)) {
                    ds_month_incr();
                }
                else if (ev == EV_S1_SHORT) {
                    flash_01 = 0;
                    kmode = CONF_SW_MMDD ? K_DATE_DISP : K_SET_DAY;
                }
                break;

            case K_SET_DAY:
                flash_23 = 1;
                if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast)) {
                    ds_day_incr();
                }
                else if (ev == EV_S1_SHORT) {
                    flash_23 = 0;
                    kmode = CONF_SW_MMDD ? K_SET_MONTH : K_DATE_DISP;
                }
                break;
#endif

            case K_WEEKDAY_DISP:
                dmode = M_WEEKDAY_DISP;
                if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast)) {
                    ds_weekday_incr();
                }
                else if (ev == EV_S2_SHORT) {
                    kmode = K_NORMAL;
                }
                break;

#ifdef DEBUG
            // To enter DEBUG mode, go to the SECONDS display, then hold S1 and S2 simultaneously.
            // S1 cycles through the DEBUG modes.
            // To exit DEBUG mode, hold S1 and S2 again.
            case K_DEBUG:
            case K_DEBUG2:
            case K_DEBUG3:
                dmode = M_DEBUG + kmode - K_DEBUG;
                if (ev == EV_S1_SHORT) {
                    kmode = (kmode - K_DEBUG + 1) % NUM_DEBUG + K_DEBUG;
                }
                else if (ev == EV_S1S2_LONG) {
                    kmode = K_SEC_DISP;
                }
                break;
#endif

            case K_SEC_DISP:
                dmode = M_SEC_DISP;
                if (ev == EV_S1_SHORT) {
                    kmode = K_NORMAL;
                }
                else if (ev == EV_S2_SHORT) {
                    ds_sec_zero();
                }
#ifdef DEBUG
                else if (ev == EV_S1S2_LONG) {
                    kmode = K_DEBUG;
                }
#endif
                break;

            case K_NORMAL:
            default:
                flash_01 = 0;
                flash_23 = 0;

                dmode = M_NORMAL;

                if (ev == EV_S1_SHORT) {
                    kmode = K_SEC_DISP;
                }
                else if (ev == EV_S1_LONG) {
                    kmode = K_SET_HOUR;
                }
                else if (ev == EV_S2_SHORT) {
                    kmode = K_TEMP_DISP;
                }
#ifdef stc15w408as
                else if (ev == EV_S3_LONG) {
                    LED = !LED;
                }
#endif
        };

        // display execution tree

        clearTmpDisplay();

        switch (dmode) {
            case M_NORMAL:
                if (!flash_01 || blinker_fast || S2_LONG) {
                    if (!H12_24) {
                        filldisplay(0, (rtc_table[DS_ADDR_HOUR] >> 4) & (DS_MASK_HOUR24_TENS >> 4), 0); // tenhour
                    }
                    else {
                        if (H12_TH) {
                            filldisplay( 0, 1, 0);// tenhour in case AMPM mode is on, then '1' only is H12_TH is on
                        }
                    }
                    filldisplay(1, rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR_UNITS, blinker_slow);
                }
                else {
                    dotdisplay(1, blinker_fast);
                }

                if (!flash_23 || blinker_fast || S2_LONG) {
                    filldisplay(2, (rtc_table[DS_ADDR_MINUTES] >> 4) & (DS_MASK_MINUTES_TENS >> 4), blinker_slow);	//tenmin
                    filldisplay(3, rtc_table[DS_ADDR_MINUTES] & DS_MASK_MINUTES_UNITS, H12_24 & H12_PM);  		//min
                }
                else {
                    dotdisplay(2, blinker_slow);
                    dotdisplay(3, H12_24 & H12_PM);	// dot3 if AMPM mode and PM=1
                }
                break;

            case M_SET_HOUR_12_24:
                if (!H12_24) {
                    filldisplay(1, 2, 0); filldisplay(2, 4, 0);
                } else {
                    filldisplay(1, 1, 0); filldisplay(2, 2, 0);
                }
                filldisplay(3, LED_h, 0);
                break;

            case M_SEC_DISP:
                dotdisplay(0, blinker_slow);
                dotdisplay(1, blinker_slow);
                filldisplay(2,(rtc_table[DS_ADDR_SECONDS] >> 4) & (DS_MASK_SECONDS_TENS >> 4), 0);
                filldisplay(3, rtc_table[DS_ADDR_SECONDS] & DS_MASK_SECONDS_UNITS, 0);
                break;

#ifndef WITHOUT_DATE
            case M_DATE_DISP:
                if (!flash_01 || blinker_fast || S2_LONG) {
                    if (!CONF_SW_MMDD) {
                        filldisplay( 0, rtc_table[DS_ADDR_MONTH] >> 4, 0);// tenmonth ( &MASK_TENS useless, as MSB bits are read as '0')
                        filldisplay( 1, rtc_table[DS_ADDR_MONTH] & DS_MASK_MONTH_UNITS, 1);
                    }
                    else {
                        filldisplay( 2, rtc_table[DS_ADDR_MONTH] >> 4, 0);// tenmonth ( &MASK_TENS useless, as MSB bits are read as '0')
                        filldisplay( 3, rtc_table[DS_ADDR_MONTH] & DS_MASK_MONTH_UNITS, 0);
                    }
                }
                else {
                    dotdisplay(1, 1);
                }

                if (!flash_23 || blinker_fast || S2_LONG) {
                    if (!CONF_SW_MMDD) {
                        filldisplay( 2, rtc_table[DS_ADDR_DAY] >> 4, 0); // tenday   ( &MASK_TENS useless)
                        filldisplay( 3, rtc_table[DS_ADDR_DAY] & DS_MASK_DAY_UNITS, 0);     // day
                    }
                    else {
                        filldisplay( 0, rtc_table[DS_ADDR_DAY] >> 4, 0); // tenday   ( &MASK_TENS useless)
                        filldisplay( 1, rtc_table[DS_ADDR_DAY] & DS_MASK_DAY_UNITS, 1);     // day
                    }
                }
                break;
#endif

            case M_WEEKDAY_DISP:
                filldisplay( 1, LED_DASH, 0);
                filldisplay( 2, rtc_table[DS_ADDR_WEEKDAY], 0); //weekday ( &MASK_UNITS useless, all MSBs are '0')
                filldisplay( 3, LED_DASH, 0);
                break;

            case M_TEMP_DISP:
                filldisplay( 0, ds_int2bcd_tens(temp), 0);
                filldisplay( 1, ds_int2bcd_ones(temp), 0);
                filldisplay( 2, CONF_C_F ? LED_f : LED_c, 1);
                // if (temp<0) filldisplay( 3, LED_DASH, 0);  -- temp defined as uint16, cannot be <0
                break;

#ifdef DEBUG
            case M_DEBUG:
            {
                // seconds, loop counter, blinkers, S1/S2, keypress events
                uint8_t cc = count;
                if (S1_PRESSED || S2_PRESSED) {
                    filldisplay( 0, S1_PRESSED || S2_PRESSED ? LED_DASH : LED_BLANK, ev == EV_S1_SHORT || ev == EV_S2_SHORT);
                    filldisplay( 1, S1_LONG || S2_LONG ? LED_DASH : LED_BLANK, ev == EV_S1_LONG || ev == EV_S2_LONG);
                } else {
                    filldisplay(0, (rtc_table[DS_ADDR_SECONDS] >> 4) & (DS_MASK_SECONDS_TENS >> 4), 0);
                    filldisplay(1, rtc_table[DS_ADDR_SECONDS] & DS_MASK_SECONDS_UNITS, blinker_slow );
                }
                filldisplay(2, cc >> 4 & 0x0F, ev != EV_NONE);
                filldisplay(3, cc & 0x0F, blinker_slow & blinker_fast);
                break;
            }
            case M_DEBUG2:
            {
                // photoresistor adc and lightval
                uint8_t adc = getADCResult8(ADC_LIGHT);
                uint8_t lv = lightval;
                filldisplay( 0, adc >> 4, 0);
                filldisplay( 1, adc & 0x0F, 0);
                filldisplay( 2, lv >> 4, 1);
                filldisplay( 3, lv & 0x0F, 0);
                break;
            }
            case M_DEBUG3:
            {
                // thermistor adc
                uint16_t rt = getADCResult(ADC_TEMP);
                filldisplay( 0, rt >> 12, 0);
                filldisplay( 1, rt >> 8 & 0x0F, 0);
                filldisplay( 2, rt >> 4 & 0x0F, 0);
                filldisplay( 3, rt & 0x0F, 0);
                break;
            }
#endif
        }

        __critical {
            updateTmpDisplay();
        }

        count++;
        WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
