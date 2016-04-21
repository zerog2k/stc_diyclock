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

// clear wdt
#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4)

// alias for relay and buzzer outputs, using relay to drive led for indication of main loop status
#define RELAY   P1_4
#define BUZZER  P1_5
    
// adc channels for sensors
#define ADC_LIGHT 6
#define ADC_TEMP  7

// three steps of dimming. Photoresistor adc value is 0-255. Lower values = brighter.
#define DIM_HI  100
#define DIM_LO  190

// button switch aliases
#define SW2     P3_0
#define S2      1
#define SW1     P3_1
#define S1      0

// button press states
#define PRESS_NONE   0
#define PRESS_SHORT  1
#define PRESS_LONG   2

// display mode states
enum display_mode {
    M_NORMAL,
    M_SET_HOUR,
    M_SET_MINUTE,
    M_SET_HOUR_12_24,
    M_TEMP_DISP,
    M_DATE_DISP,
    M_SET_MONTH,
    M_SET_DAY,
    M_WEEKDAY_DISP
};

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
uint16_t temp = 0;    // temperature sensor value
uint8_t lightval = 0;   // light sensor value
struct ds1302_rtc rtc;
struct ram_config config;

volatile uint8_t displaycounter = 0;
uint8_t dbuf[4] = {0,0,0,0};     // led display buffer
uint8_t dmode = M_NORMAL;   // display mode state
__bit  display_colon = 0;         // flash colon
__bit  flash_hours = 0;
__bit  flash_minutes = 0;
__bit  flash_month = 0;
__bit  flash_day = 0;

volatile uint8_t debounce[2] = {0xFF, 0xFF};      // switch debounce buffer
volatile uint8_t switchcount[2] = {0, 0};

void timer0_isr() __interrupt 1 __using 1
{
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
        P3 &= ~((0x1 << digit) << 2);  
    }
    displaycounter++;
    // done    
}

void timer1_isr() __interrupt 3 __using 1 {
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
    if (switchcount[keynum] > 150) {
        _delay_ms(30);
        return PRESS_LONG;  // ~1.5 sec
    }
    if (switchcount[keynum]) {
        _delay_ms(50);
        return PRESS_SHORT; // ~100 msec
    }
    return PRESS_NONE;
}

int8_t gettemp(uint16_t raw) {
    // formula for ntc adc value to approx C
    return 76 - raw * 64 / 637;
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
    // init/read ram config
    ds_ram_config_init((uint8_t *) &config);    
    
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

      // run every ~2 secs
      if (count % 16 == 0) {
          lightval = getADCResult8(ADC_LIGHT);
          temp = gettemp( getADCResult(ADC_TEMP) );

          // dimming modulus selection
          if (lightval < DIM_HI)
              lightval = 4;
          else if (lightval < DIM_LO)
              lightval = 8;
          else
              lightval = 64;
      }   

      ds_readburst((uint8_t *) &rtc); // read rtc

      // display decision tree
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
              break;
              
          case M_SET_MINUTE:
              flash_hours = 0;
              flash_minutes = !flash_minutes;
              if (! flash_minutes) {
                  if (getkeypress(S2)) {
                      ds_minutes_incr(&rtc);
                  }
                  if (getkeypress(S1))
                      dmode = M_SET_HOUR_12_24;
              }
              break;

          case M_SET_HOUR_12_24:
              if (getkeypress(S2))
                  ds_hours_12_24_toggle(&rtc);
              if (getkeypress(S1))
                  dmode = M_NORMAL;
              break;
              
          case M_TEMP_DISP:
              // TODO: display temp
          if (getkeypress(S2))
                  dmode = M_DATE_DISP;
              break;
                        
          case M_DATE_DISP:
              if (getkeypress(S1))
                  dmode = M_SET_MONTH;
              if (getkeypress(S2))
                  dmode = M_WEEKDAY_DISP;                        
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
              break;
              
          case M_WEEKDAY_DISP:
              if (getkeypress(S1))
                  ds_weekday_incr(&rtc);
              if (getkeypress(S2))
                  dmode = M_NORMAL;
              break;
              
          case M_NORMAL:          
          default:
              flash_hours = 0;
              flash_minutes = 0;
              if (count % 8 < 2)
                  display_colon = 1; // flashing colon
              else
                  display_colon = 0;

              if (getkeypress(S1) == PRESS_LONG && getkeypress(S2) == PRESS_LONG)
                  ds_reset_clock();   
              
              if (getkeypress(S1 == PRESS_SHORT)) {
                  dmode = M_SET_HOUR;
              }
              if (getkeypress(S2 == PRESS_SHORT)) {
                  dmode = M_TEMP_DISP;
              }
      
      };

      // display execution tree
      
      switch (dmode) {
          case M_NORMAL:
          case M_SET_HOUR:
          case M_SET_MINUTE:
              if (flash_hours) {
                  filldisplay(dbuf, 0, LED_BLANK, 0);
                  filldisplay(dbuf, 1, LED_BLANK, display_colon);
              } else {
                  if (rtc.h24.hour_12_24 == HOUR_24) {
                      filldisplay(dbuf, 0, rtc.h24.tenhour, 0);
                  } else {
                      filldisplay(dbuf, 0, rtc.h12.tenhour ? rtc.h12.tenhour : LED_BLANK, 0);
                  }                  
                  filldisplay(dbuf, 1, rtc.h12.hour, display_colon);      
              }
  
              if (flash_minutes) {
                  filldisplay(dbuf, 2, LED_BLANK, display_colon);
                  filldisplay(dbuf, 3, LED_BLANK, (rtc.h12.hour_12_24) ? rtc.h12.pm : 0);  
              } else {
                  filldisplay(dbuf, 2, rtc.tenminutes, display_colon);
                  filldisplay(dbuf, 3, rtc.minutes, (rtc.h12.hour_12_24) ? rtc.h12.pm : 0);  
              }
              break;

          case M_SET_HOUR_12_24:
              filldisplay(dbuf, 0, LED_BLANK, 0);
              if (rtc.h24.hour_12_24 == HOUR_24) {
                  filldisplay(dbuf, 1, 2, 0);
                  filldisplay(dbuf, 2, 4, 0);
              } else {
                  filldisplay(dbuf, 1, 1, 0);
                  filldisplay(dbuf, 2, 2, 0);                  
              }
              filldisplay(dbuf, 3, LED_h, 0);
              break;
              
          case M_DATE_DISP:
          case M_SET_MONTH:
          case M_SET_DAY:
              if (flash_month) {
                  filldisplay(dbuf, 0, LED_BLANK, 0);
                  filldisplay(dbuf, 1, LED_BLANK, 1);
              } else {
                  filldisplay(dbuf, 0, rtc.tenmonth, 0);
                  filldisplay(dbuf, 1, rtc.month, 1);          
              }
              if (flash_day) {
                  filldisplay(dbuf, 2, LED_BLANK, 0);
                  filldisplay(dbuf, 3, LED_BLANK, 0);              
              } else {
                  filldisplay(dbuf, 2, rtc.tenday, 0);
                  filldisplay(dbuf, 3, rtc.day, 0);              
              }     
              break;
                   
          case M_WEEKDAY_DISP:
              filldisplay(dbuf, 0, LED_BLANK, 0);
              filldisplay(dbuf, 1, LED_DASH, 0);
              filldisplay(dbuf, 2, rtc.weekday, 0);
              filldisplay(dbuf, 3, LED_DASH, 0);
              break;
              
          case M_TEMP_DISP:
              filldisplay(dbuf, 0, ds_int2bcd_tens(temp), 0);
              filldisplay(dbuf, 1, ds_int2bcd_ones(temp), 0);
              filldisplay(dbuf, 2, LED_c, 1);
              filldisplay(dbuf, 3, (temp > 0) ? LED_BLANK : LED_DASH, 0);  
              break;                  
      }
                  
      _delay_ms(40);
      count++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
