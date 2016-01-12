//
// STC15F204EA blinky example
// inspired by http://jjmz.free.fr/?p=179 http://jjmz.free.fr/?p=191
// and stc15f204ea datasheet
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>
#include "adc.h"
#include "ds1302.h"
#include "led.h"
#include "./soft_serial/serial.h"
    
#define FOSC    11059200

#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4) // clear wdt
/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */

#define printf printf_fast     // see sdcc user guide

#define RELAY   P1_4
#define BUZZER  P1_5
    
#define ADC_LIGHT 6
#define ADC_TEMP  7

#define SW2     P3_0
#define SW1     P3_1

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
int count = 0;
const char* startstring = "\nSTC15F204EA DIY Clock starting up...\n";
unsigned int tempval = 0;    // temperature sensor value
uint8_t lightval = 0;   // light sensor value
volatile uint8_t displaycounter = 0;
struct ds1302_rtc rtc;
uint8_t display[4] = {0,0,0,0};     // led display buffer

/* Timer0 ISR */
void tm0_isr() __interrupt 1 __using 1
{
    // led display refresh cycle
    // select one of four digits
    uint8_t digit = displaycounter % 4; 

    // turn off all digits, set high    
    P3 |= 0x3C;

    // auto dimming, skip lighting for some cycles
    if (displaycounter > 247  || displaycounter > lightval) {
        // fill digits
        P2 = display[digit];
        // turn on selected digit, set low
        P3 &= ~((0x1 << digit) << 2);  
    }
    displaycounter++;
    // done    
}

void Timer0Init(void)		//10ms@11.0592MHz
{
    AUXR &= 0x7F;		//Timer clock is 12T mode
    TMOD &= 0xF0;		//Set timer work mode
    TL0 = 0xA0;		//Initial timer value
    TH0 = 0xFF;		//Initial timer value
    TF0 = 0;		//Clear TF0 flag
    TR0 = 1;		//Timer0 start run
    ET0 = 1;
}


int main()
{
    // SETUP
    // set ds1302 pins to open-drain output, they already have strong pullups
    P1M1 |= ((1 << 0) | (1 << 1) | (1 << 2));
    P1M0 |= ((1 << 0) | (1 << 1) | (1 << 2));
    
	_delay_ms(100);
      
    /* init the software uart */
    UART_INIT();

    /* simple greeting message */
    printf("%s", startstring);
    
    RELAY = 1;
    //BUZZER = 1;
        
    // init rtc
    ds_writebyte(DS_ADDR_WP, 0); // clear WP (Write Protect)
    ds_writebyte(DS_ADDR_SECONDS, 0); // clear CH (Clock Halt), start clock
    
    Timer0Init(); // display refresh
    
    // LOOP
    while(1)
    {             
      RELAY = 0;
      //BUZZER = 0;
      _delay_ms(255);

      RELAY = 1;
      //BUZZER = 1;
      printf("\ncounter: %d \n", count);
      
      lightval = getADCResult8(ADC_LIGHT);
      tempval = getADCResult(ADC_TEMP);
      
      printf("adc: light raw: %03d, temperature raw: %03d\n", lightval, tempval);   
      
      /*
      printf("seconds: %02x, minutes: %02x, hour: %02x, day: %02x, month: %02x, weekday: %02x, year: %02x\n", 
          ds_readbyte(DS_ADDR_SECONDS),  ds_readbyte(DS_ADDR_MINUTES),  ds_readbyte(DS_ADDR_HOUR),
          ds_readbyte(DS_ADDR_DAY),  ds_readbyte(DS_ADDR_MONTH),  ds_readbyte(DS_ADDR_WEEKDAY), ds_readbyte(DS_ADDR_YEAR));
      printf("DS_ADDR_TCSDS: %02x\n", ds_readbyte(DS_ADDR_TCSDS));
      */
      
      ds_readburst((uint8_t *) &rtc); // read rtc
      printf("yy mm dd hh mm ss am/pm 24/12 ww \n%d%d %d%d %d%d %d%d %d%d %d%d     %d     %d  %d\n",
          rtc.tenyear, rtc.year, rtc.tenmonth, rtc.month, rtc.tenday, rtc.day, rtc.h12.tenhour, rtc.h12.hour, 
          rtc.tenminutes, rtc.minutes, rtc.tenseconds, rtc.seconds, rtc.h12.pm, rtc.h12.hour_12_24, rtc.weekday);
      //
      filldisplay(display, rtc.h12.tenhour, 0);
      filldisplay(display, rtc.h12.hour, 1);
      filldisplay(display, rtc.tenminutes, 2);
      filldisplay(display, rtc.minutes, 3);
      
      //printf("%02x %02x %02x %02x\n", display[0], display[1], display[2], display[3]);
      
      _delay_ms(255);
      _delay_ms(255);
      count++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
