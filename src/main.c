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

// display mode states
enum keyboard_mode {
    K_NORMAL,
    K_WAIT_S1,
    K_WAIT_S2,
    K_SET_HOUR,
    K_SET_MINUTE,
    K_SET_HOUR_12_24,
    K_SEC_DISP,
    K_TEMP_DISP,
    K_DATE_DISP,
    K_DATE_SWDISP,
    K_SET_MONTH,
    K_SET_DAY,
    K_WEEKDAY_DISP,
    K_DEBUG
};

// display mode states
enum display_mode {
    M_NORMAL,
    M_SET_HOUR_12_24,
    M_SEC_DISP,
    M_TEMP_DISP,
    M_DATE_DISP,
    M_WEEKDAY_DISP,
    M_DEBUG
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
uint8_t  count;     // was uint16 - 8 seems to be enough
uint16_t temp;      // temperature sensor value
uint8_t  lightval;  // light sensor value

volatile uint8_t displaycounter;

uint8_t dmode = M_NORMAL;     // display mode state
uint8_t kmode = K_NORMAL;
uint8_t smode,lmode;

__bit  display_colon;         // flash colon
__bit  flash_01;
__bit  flash_23;
__bit  beep = 1;

__bit  S1_LONG;
__bit  S1_PRESSED;
__bit  S2_LONG;
__bit  S2_PRESSED;

volatile uint8_t debounce[2];      // switch debounce buffer
volatile uint8_t switchcount[2];

void timer0_isr() __interrupt 1 __using 1
{
    // display refresh ISR
    // cycle thru digits one at a time
    uint8_t digit = displaycounter & 3; 

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
    // done    
}

#define SW_CNTMAX 80

void timer1_isr() __interrupt 3 __using 1 {
    // debounce ISR
    
    // increment count if settled closed
    if ((debounce[0] & 0x0F) == 0x00)    
        {S1_PRESSED=1; switchcount[0]++;}
    else
        {S1_PRESSED=0; switchcount[0]=0;}
    
    if ((debounce[1] & 0x0F) == 0x00)
        {S2_PRESSED=1; switchcount[1]++;}
    else
        {S2_PRESSED=0; switchcount[1]=0;}

    // debouncing stuff
    // keep resetting halfway if held long
    if (switchcount[0] > SW_CNTMAX)
        {switchcount[0] = SW_CNTMAX; S1_LONG=1;}
    if (switchcount[1] > SW_CNTMAX)
        {switchcount[1] = SW_CNTMAX; S2_LONG=1;}

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

#define getkeypress(a) a##_PRESSED

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
            
    // init rtc
    ds_init();
    // init/read ram config
    ds_ram_config_init();
    
    // uncomment in order to reset minutes and hours to zero.. Should not need this.
    //ds_reset_clock();    
    
    Timer0Init(); // display refresh
    Timer1Init(); // switch debounce
    
    // LOOP
    while(1)
    {   
            
      RELAY = 0;
      _delay_ms(60);

      RELAY = 1;

      // run every ~1 secs
      if ((count & 3) == 0) {
          lightval = getADCResult8(ADC_LIGHT) >> 3;
          //temp = gettemp(getADCResult(ADC_TEMP)) + config.temp_offset;
          temp = gettemp(getADCResult(ADC_TEMP)) + (config_table[2]&0x7) - 4;

          // constrain dimming range
          if (lightval < 4) 
              lightval = 4;

      }       

      ds_readburst(); // read rtc

      // keyboard decision tree
      switch (kmode) {
          
          case K_SET_HOUR:
              display_colon = 1;
              flash_01 = !flash_01;
              if (! flash_01) {
                  if (getkeypress(S2)) ds_hours_incr();
                  if (getkeypress(S1)) kmode = K_SET_MINUTE;
              }
              break;
              
          case K_SET_MINUTE:
              flash_01 = 0;
              flash_23 = !flash_23;
              if (! flash_23) {
                  if (getkeypress(S2)) ds_minutes_incr();
                  if (getkeypress(S1)) kmode = K_SET_HOUR_12_24;
              }
              break;

          case K_SET_HOUR_12_24:
	      dmode=M_SET_HOUR_12_24;
              if (getkeypress(S2)) ds_hours_12_24_toggle();
              if (getkeypress(S1)) kmode = K_NORMAL;
              break;
              
          case K_TEMP_DISP:
	      dmode=M_TEMP_DISP;
              if (getkeypress(S1))
                  { uint8_t offset=config_table[2]&0x7; offset++; offset&=7; config_table[2]=(config_table[2]&0xF8)|offset; }
              if (getkeypress(S2)) kmode = K_DATE_DISP;
              break;
                        
          case K_DATE_DISP:
              dmode=M_DATE_DISP;
              if (getkeypress(S1)) {kmode=K_WAIT_S1; lmode=CONF_SW_MMDD?K_SET_DAY:K_SET_MONTH; smode=K_DATE_SWDISP; }
              if (getkeypress(S2)) kmode = K_WEEKDAY_DISP;                        
              break;

	  case K_DATE_SWDISP:
	      CONF_SW_MMDD=!CONF_SW_MMDD;
              kmode=K_DATE_DISP;
	      break;
              
          case K_SET_MONTH:
              flash_01 = !flash_01;
              if (! flash_01) {
                  if (getkeypress(S2)) { ds_month_incr(); }
                  if (getkeypress(S1)) { flash_01 = 0; kmode = CONF_SW_MMDD?K_DATE_DISP:K_SET_DAY; }
              }
              break;
              
          case K_SET_DAY:
              flash_23 = !flash_23;
              if (! flash_23) {
                  if (getkeypress(S2)) { ds_day_incr(); }
                  if (getkeypress(S1)) { flash_23 = 0; kmode = CONF_SW_MMDD?K_SET_MONTH:K_DATE_DISP;
                  }
              }
              break;
              
          case K_WEEKDAY_DISP:
              dmode=M_WEEKDAY_DISP;
              if (getkeypress(S1)) ds_weekday_incr();
              if (getkeypress(S2)) kmode = K_NORMAL;
              break;
          
	  case K_DEBUG:
	      if (count>100) kmode = K_NORMAL;
              if (S1_PRESSED||S2_PRESSED) count=0;
	      break;

	  case K_SEC_DISP:
	      dmode=M_SEC_DISP;
              if (count % 10 < 4)
                  display_colon = 1; // flashing colon
              else
                  display_colon = 0;
	      if (getkeypress(S1) || (count>100)) { kmode = K_NORMAL; }
	      if (getkeypress(S2)) { rtc_table[DS_ADDR_SECONDS]=0; }
	      break;

	  case K_WAIT_S1:
	      if (!S1_PRESSED) {
               if (S1_LONG) {S1_LONG=0;kmode=lmode;}
                      else  {kmode=smode;}
               }
	      break;

	  case K_WAIT_S2:
	      if (!S2_PRESSED) {
               if (S2_LONG) {S2_LONG=0;kmode=lmode;}
                      else  {kmode=smode;}
               }
	      break;

          case K_NORMAL:          
          default:
              flash_01 = 0;
              flash_23 = 0;
              if (count % 10 < 4)
                  display_colon = 1; // flashing colon
              else
                  display_colon = 0;

	      dmode=M_NORMAL;

	      if (S1_PRESSED) { kmode = K_WAIT_S1; lmode=K_SET_HOUR; smode=K_SEC_DISP;  }
              if (S2_PRESSED) { kmode = K_WAIT_S2; lmode=K_DEBUG;    smode=K_TEMP_DISP; }
      
      };

      // display execution tree
     
      clearTmpDisplay();

      switch (dmode) {
          case M_NORMAL:
              if (flash_01) {
		  dotdisplay(1,display_colon);
              } else {
		  if (!H12_24) { 
                      filldisplay( 0, (rtc_table[DS_ADDR_HOUR]>>4)&3, 0);	// tenhour 
                  } else {
                      if (H12_TH) filldisplay( 0, 1, 0);
                  }                  
                  filldisplay( 1, rtc_table[DS_ADDR_HOUR]&0xF, display_colon);      
              }
  
              if (flash_23) {
	          dotdisplay(2,display_colon);
                  dotdisplay(3,H12_24&H12_PM);
              } else {
                  filldisplay( 2, (rtc_table[DS_ADDR_MINUTES]>>4)&7, display_colon);	//tenmin
                  filldisplay( 3, rtc_table[DS_ADDR_MINUTES]&0xF, H12_24 & H12_PM);  	//min
              }
              break;

          case M_SET_HOUR_12_24:
              if (!H12_24) {
                  filldisplay( 1, 2, 0); filldisplay( 2, 4, 0);
              } else {
                  filldisplay( 1, 1, 0); filldisplay( 2, 2, 0);                  
              }
              filldisplay( 3, LED_h, 0);
              break;

	  case M_SEC_DISP:
	      dotdisplay(0,display_colon);
	      dotdisplay(1,display_colon);
	      filldisplay(2,(rtc_table[DS_ADDR_SECONDS]>>4)&7,0);
	      filldisplay(3,rtc_table[DS_ADDR_SECONDS]&15,0);
	      break;
              
          case M_DATE_DISP:
              if (flash_01) {
		  dotdisplay(1,1);
              } else {
                 if (!CONF_SW_MMDD) {
                  filldisplay( 0, rtc_table[DS_ADDR_MONTH]>>4, 0);	// tenmonth ( &0x01 useless, as MSB bits are read as '0')
                  filldisplay( 1, rtc_table[DS_ADDR_MONTH]&0xF, 1); }         
                 else {
                  filldisplay( 2, rtc_table[DS_ADDR_MONTH]>>4, 0);	// tenmonth ( &0x01 useless, as MSB bits are read as '0')
                  filldisplay( 3, rtc_table[DS_ADDR_MONTH]&0xF, 0); }         
              }
              if (!flash_23) {
                 if (!CONF_SW_MMDD) {
                  filldisplay( 2, rtc_table[DS_ADDR_DAY]>>4, 0);	// tenday   ( &0x03 useless)
                  filldisplay( 3, rtc_table[DS_ADDR_DAY]&0xF, 0); }     // day       
                 else {
                  filldisplay( 0, rtc_table[DS_ADDR_DAY]>>4, 0);	// tenday   ( &0x03 useless)
                  filldisplay( 1, rtc_table[DS_ADDR_DAY]&0xF, 1); }     // day       
              }     
              break;
                   
          case M_WEEKDAY_DISP:
              filldisplay( 1, LED_DASH, 0);
              filldisplay( 2, rtc_table[DS_ADDR_WEEKDAY], 0);		//weekday ( &0x07 useless)
              filldisplay( 3, LED_DASH, 0);
              break;
              
          case M_TEMP_DISP:
              filldisplay( 0, ds_int2bcd_tens(temp), 0);
              filldisplay( 1, ds_int2bcd_ones(temp), 0);
              filldisplay( 2, CONF_C_F ? LED_f : LED_c, 1);
              // if (temp<0) filldisplay( 3, LED_DASH, 0);  -- temp defined as uint16, cannot be <0
              break;                  

	  case M_DEBUG:
              filldisplay( 0, switchcount[0]>>4, S1_LONG);
              filldisplay( 1, switchcount[0]&15, S1_PRESSED);
              filldisplay( 2, switchcount[1]>>4, S2_LONG);
              filldisplay( 3, switchcount[1]&15, S2_PRESSED);
	      break;
      }

      updateTmpDisplay();
                  
      // save ram config
      ds_ram_config_write(); 
      _delay_ms(40);
      count++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
