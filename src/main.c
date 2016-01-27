//
// STC15F204EA DIY LED Clock
// Copyright 2016, Jens Jensen
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>
#include "adc.h"
#include "ds1302.h"
#include "led.h"
    
#define FOSC    11059200

#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4) // clear wdt

#define RELAY   P1_4
#define BUZZER  P1_5
    
#define ADC_LIGHT 6
#define ADC_TEMP  7

#define DIM_HI  100
#define DIM_LO  200

#define SW2     P3_0
#define S2      1
#define SW1     P3_1
#define S1      0

#define PRESS_NONE   0
#define PRESS_SHORT  1
#define PRESS_LONG   2

enum {
    M_NORMAL,
    M_SET_HOUR,
    M_SET_MINUTE,
    M_TEMP_DISP,
    M_TEMP_ADJUST,
    M_DATE_DISP,
    M_SET_MONTH,
    M_SET_DAY,
    M_WEEKDAY_DISP
} display_mode;

/* ------------------------------------------------------------------------- */

void _delay_ms(uint8_t ms)
{	
    // i,j selected for fosc 11.0592MHz, using oscilloscope
    // the stc-isp tool gives inaccurate values (perhaps for C51 vs sdcc?)
    // max 255 ms
    uint8_t i, j;
    do {
    	i = 4;
    	j = 200;
    	do
    	{
    		while (--j);
    	} while (--i);
    } while (--ms);
}

// GLOBALS
uint8_t i;
uint16_t count = 0;
uint16_t tempval = 0;    // temperature sensor value
uint8_t lightval = 0;   // light sensor value
struct ds1302_rtc rtc;

volatile uint8_t displaycounter = 0;
uint8_t display[4] = {0,0,0,0};     // led display buffer
uint8_t dmode = M_NORMAL;   // display mode state
__bit  display_colon = 0;         // flash colon
__bit  display_time = 1;
__bit  display_date = 0;
__bit  display_weekday = 0;
__bit  flash_hours = 0;
__bit  flash_minutes = 0;
__bit  flash_month = 0;
__bit  flash_day = 0;

volatile uint8_t debounce[2] = {0xFF, 0xFF};      // switch debounce buffer
volatile uint8_t switchcount[2] = {0, 0};

/* Timer0 ISR */
void tm0_isr() __interrupt 1 __using 1
{
    // display refresh ISR
    // cycle thru digits one at a time
    uint8_t digit = displaycounter % 4; 

    // turn off all digits, set high    
    P3 |= 0x3C;

    // auto dimming, skip lighting for some cycles
    if (displaycounter % lightval < 4 ) {
        // fill digits
        P2 = display[digit];
        // turn on selected digit, set low
        P3 &= ~((0x1 << digit) << 2);  
    }
    displaycounter++;
    // done    
}

void tm1_isr() __interrupt 3 __using 1 {
    // debounce ISR
    
    // debouncing stuff
    // keep resetting halfway if held long
    if (switchcount[0] > 250)
        switchcount[0] = 100;
    if (switchcount[1] > 250)
        switchcount[1] = 100;

    // increment count if settled closed
    if (debounce[0] == 0x00)    
        switchcount[0]++;
    else
        switchcount[0] = 0;
    
    if (debounce[1] == 0x00)
        switchcount[1]++;
    else
        switchcount[1] = 0;

    // read switch positions into sliding 8-bit window
    debounce[0] = (debounce[0] << 1) | SW1;
    debounce[1] = (debounce[1] << 1) | SW2;  
}

void Timer0Init(void)		//100us @ 11.0592MHz
{
    TL0 = 0xA3;		//Initial timer value
    TH0 = 0xFF;		//Initial timer value
    TF0 = 0;		//Clear TF0 flag
    TR0 = 1;		//Timer0 start run
    ET0 = 1;        // enable timer0 interrupt
    EA = 1;         // global interrupt enable
}

void Timer1Init(void)		//10ms @ 11.0592MHz
{
	TL1 = 0xD5;		//Initial timer value
	TH1 = 0xDB;		//Initial timer value
	TF1 = 0;		//Clear TF1 flag
	TR1 = 1;		//Timer1 start run
    ET1 = 1;    // enable Timer1 interrupt
    EA = 1;     // global interrupt enable
}

uint8_t getkeypress(uint8_t keynum)
{
    if (switchcount[keynum] > 150)
        return PRESS_LONG;  // ~1.5 sec
    if (switchcount[keynum] > 2) 
        return PRESS_SHORT; // ~100 msec
    return PRESS_NONE;
}


/*********************************************/
int main()
{
    // SETUP
    // set ds1302, photoresistor, & ntc pins to open-drain output, already have strong pullups
    P1M1 |= (1 << 0) | (1 << 1) | (1 << 2) | (1<<6) | (1<<7);
    P1M0 |= (1 << 0) | (1 << 1) | (1 << 2) | (1<<6) | (1<<7);
    
	_delay_ms(100);
      
    
    RELAY = 1;
    //BUZZER = 1;
        
    // init rtc
    ds_init();
    
    // uncomment in order to reset minutes and hours to zero.. Should not need this.
    //ds_reset_clock();    
    
    Timer0Init(); // display refresh
    Timer1Init(); // switch debounce
    
    // LOOP
    while(1)
    {             
      RELAY = 0;
      _delay_ms(100);

      RELAY = 1;

      if (count % 8 == 0) {
          lightval = getADCResult8(ADC_LIGHT);
          tempval = getADCResult(ADC_TEMP);

          if (lightval < DIM_HI)
              lightval = 4;
          else if (lightval < DIM_LO)
              lightval = 8;
          else
              lightval = 32;
      }   

      ds_readburst((uint8_t *) &rtc); // read rtc

      switch (dmode) {
          
          case M_SET_HOUR:
              display_colon = 1;
              flash_hours = !flash_hours;
              if (! flash_hours) {
                  if (getkeypress(S2)) {
                      ds_hours_incr(&rtc);
                  }
                  if (getkeypress(S1))
                      dmode = M_SET_MINUTE;
              }
              _delay_ms(20);
              break;
              
          case M_SET_MINUTE:
              flash_hours = 0;
              display_colon = 1;
              flash_minutes = !flash_minutes;
              if (! flash_minutes) {
                  if (getkeypress(S2)) {
                      ds_minutes_incr(&rtc);
                  }
                  if (getkeypress(S1))
                      dmode = M_NORMAL;
              }
              _delay_ms(20);              
              break;
              
          case M_TEMP_DISP:
              // TODO: display temp
              break;
              
          case M_TEMP_ADJUST:
              // TODO: adjust temp
              break;
              
          case M_DATE_DISP:
              display_time = 0;
              display_date = 1;
              if (getkeypress(S1))
                  dmode = M_SET_MONTH;
              if (getkeypress(S2))
                  dmode = M_WEEKDAY_DISP;                        
              _delay_ms(50);              
              break;
              
          case M_SET_MONTH:
              flash_month = !flash_month;
              if (! flash_month) {
                  if (getkeypress(S2)) {
                      ds_month_incr(&rtc);
                  }
                  if (getkeypress(S1)) {
                      flash_month = 0;
                      dmode = M_SET_DAY;
                  }
              }
              _delay_ms(20);              
              break;
              
          case M_SET_DAY:
              flash_day = !flash_day;
              if (! flash_day) {
                  if (getkeypress(S2)) {
                      ds_day_incr(&rtc);
                  }
                  if (getkeypress(S1)) {
                      flash_day = 0;
                      dmode = M_DATE_DISP;
                  }
              }
              _delay_ms(20);              
              break;
              
          case M_WEEKDAY_DISP:
              display_time = 0;
              display_date = 0;
              display_weekday = 1;
              if (getkeypress(S1))
                  ds_weekday_incr(&rtc);
              if (getkeypress(S2))
                  dmode = M_NORMAL;
              _delay_ms(50);                          
              break;
              
          case M_NORMAL:          
          default:
              flash_hours = 0;
              flash_minutes = 0;
              display_time = 1;
              display_date = 0;
              display_weekday = 0;
              if (count % 8 < 2)
                  display_colon = 1; // flashing colon
              else
                  display_colon = 0;

              if (getkeypress(S1) == PRESS_LONG && getkeypress(S2) == PRESS_LONG)
                  ds_reset_clock();   
              
              if (getkeypress(S1 == PRESS_SHORT)) {
                  dmode = M_SET_HOUR;
                  _delay_ms(20);                  
              }
              if (getkeypress(S2 == PRESS_SHORT)) {
                  dmode = M_DATE_DISP;
                  _delay_ms(20);                  
              }
      
      };

      if (display_time) {
          if (flash_hours) {
              filldisplay(display, 0, LED_BLANK, 0);
              filldisplay(display, 1, LED_BLANK, display_colon);
          } else {
              filldisplay(display, 0, (rtc.h12.hour_12_24) ? (rtc.h12.tenhour ? rtc.h12.tenhour : LED_BLANK) : rtc.h12.hour_12_24, 0);
              filldisplay(display, 1, rtc.h12.hour, display_colon);      
          }
  
          if (flash_minutes) {
              filldisplay(display, 2, LED_BLANK, display_colon);
              filldisplay(display, 3, LED_BLANK, (rtc.h12.hour_12_24) ? rtc.h12.pm : 0);  
          } else {
              filldisplay(display, 2, rtc.tenminutes, display_colon);
              filldisplay(display, 3, rtc.minutes, (rtc.h12.hour_12_24) ? rtc.h12.pm : 0);  
          }
      } else if (display_date) {
          if (flash_month) {
              filldisplay(display, 0, LED_BLANK, 0);
              filldisplay(display, 1, LED_BLANK, 1);
          } else {
              filldisplay(display, 0, rtc.tenmonth, 0);
              filldisplay(display, 1, rtc.month, 1);          
          }
          if (flash_day) {
              filldisplay(display, 2, LED_BLANK, 0);
              filldisplay(display, 3, LED_BLANK, 0);              
          } else {
              filldisplay(display, 2, rtc.tenday, 0);
              filldisplay(display, 3, rtc.day, 0);              
          }          
      } else if (display_weekday) {
          filldisplay(display, 0, LED_BLANK, 0);
          filldisplay(display, 1, LED_DASH, 0);
          filldisplay(display, 2, rtc.weekday, 0);
          filldisplay(display, 3, LED_DASH, 0);
      }
                  
      _delay_ms(40);
      count++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
