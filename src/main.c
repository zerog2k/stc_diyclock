//
// STC15F204EA blinky example
// inspired by http://jjmz.free.fr/?p=179 http://jjmz.free.fr/?p=191
// and stc15f204ea datasheet
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>
#include "adc.h"

#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4) // clear wdt
/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */

#include "./soft_serial/serial.h"
#define printf printf_fast     // see sdcc user guide

#define RELAY   P1_4
#define BUZZER  P1_5
    
#define LED_A1  P3_2
#define LED_A2  P3_3
#define LED_A3  P3_4
#define LED_A4  P3_5

#define LED_a   P2_0
#define LED_b   P2_1
#define LED_c   P2_2
#define LED_d   P2_3
#define LED_e   P2_4
#define LED_f   P2_5
#define LED_g   P2_6
#define LED_dp  P2_7

#define ADC_LIGHT 6
#define ADC_TEMP  7

#define SW2     P3_0
#define SW1     P3_1


/* ------------------------------------------------------------------------- */

// counter
int temp = 0;

const char* startstring = "\nSTC15F204EA starting up...\n";


// ds1302 section. TODO: move to lib
#define DS_CE    P1_0
#define DS_IO    P1_1
#define DS_SCLK  P1_2

#define DS_CMD        1 << 7
#define DS_CMD_READ   1
#define DS_CMD_WRITE  0
#define DS_CMD_RAM    1 << 6
#define DS_CMD_CLOCK  0 << 6

#define DS_ADDR_SECONDS     0
#define DS_ADDR_MINUTES     1
#define DS_ADDR_HOUR        2
#define DS_ADDR_DAY         3
#define DS_ADDR_MONTH       4
#define DS_ADDR_WEEKDAY     5
#define DS_ADDR_YEAR        6
#define DS_ADDR_WP          7
#define DS_ADDR_TCSDS       8

#define DS_BURST_MODE       31

struct ds1302_rtc {
    // 8 bytes. data fields are bcd
    
    // 00-59
    uint8_t seconds:4;      
    uint8_t tenseconds:3;   
    uint8_t clock_halt:1;

    // 00-59    
    uint8_t minutes:4;      
    uint8_t tenminutes:3;   
    uint8_t reserved1:1;
    
    union {
        struct {
            // 1-12
            uint8_t hour:4;         
            uint8_t tenhour:2;       
            uint8_t reserved2:1;
            uint8_t hour_12_24:1;    // 0=24h
        } h24;
        struct {
            // 0-23
            uint8_t hour:4;      
            uint8_t tenhour:1;
            uint8_t pm:1;           // 0=am, 1=pm;
            uint8_t reserved2:1;
            uint8_t hour_12_24:1;   // 1=12h
        } h12;
    };
      
    // 1-31
    uint8_t day:4;          
    uint8_t tenday:2;
    uint8_t reserved3:2;

    // 1-12
    uint8_t month:4;        
    uint8_t tenmonth:1;
    uint8_t reserved4:3;
    
    // 1-7
    uint8_t weekday:3;      
    uint8_t reserved5:5;
    
    // 00-99
    uint8_t year:4;         
    uint8_t tenyear:4;
    
    uint8_t reserved:7;    
    uint8_t write_protect:1;
};


struct ds1302_rtc rtc;

void ds_writebit(__bit bit) {
    _nop_; _nop_;
    DS_IO = bit;
    DS_SCLK = 1;
    _nop_; _nop_;
    DS_SCLK = 0;
}

__bit ds_readbit() {
    __bit b;
    _nop_; _nop_;
    b = DS_IO;
    DS_SCLK = 1;
    _nop_; _nop_;
    DS_SCLK = 0;
    return b;
}

uint8_t ds_readbyte(uint8_t addr) {
    // ds1302 single-byte read
    uint8_t i, b = 0;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    for (i=0; i < 8; i++) {
        ds_writebit((b >> i) & 0x01);
        _nop_; _nop_;
    }
    // read byte
    for (i=0; i < 8; i++) {
        if (ds_readbit()) 
            b |= 1 << i; // set
        else
            b &= ~(1 << i); // clear  
        _nop_; _nop_;              
    }
    DS_CE = 0;
    return b;
}

void ds_readburst(uint8_t time[8]) {
    // ds1302 burst-read 8 bytes into struct
    uint8_t i, j, b = 0;
    b = DS_CMD | DS_CMD_CLOCK | DS_BURST_MODE << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    for (i=0; i < 8; i++) {
        ds_writebit((b >> i) & 0x01);
        _nop_; _nop_;
    }
    // read bytes
    for (j=0; j < 8; j++) {
        for (i=0; i < 8; i++) {
            if (ds_readbit()) 
                b |= 1 << i; // set
            else
                b &= ~(1 << i); // clear  
            _nop_; _nop_;              
        }
        time[j] = b;
    }
    DS_CE = 0;
}

uint8_t ds_writebyte(uint8_t addr, uint8_t data) {
    // ds1302 single-byte write
    uint8_t i, b = 0;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_WRITE;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    for (i=0; i < 8; i++) {
        ds_writebit((b >> i) & 0x01);
        _nop_; _nop_;
    }
    // send data byte
    for (i=0; i < 8; i++) {
        ds_writebit((data >> i) & 0x01);
        _nop_; _nop_;
    }

    DS_CE = 0;
    return b;
}


/*----------------------------
Send ADC result to UART
----------------------------*/
void showADC(char ch)
{
	unsigned int result = getADCResult(ch);
	printf("adc chan: %d, result: %03d\n", ch, result);                   
}

//////////////////////////////
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
    
    // LOOP
    while(1)
    {             
      RELAY = 0;
      //BUZZER = 0;
      _delay_ms(250);

      RELAY = 1;
      //BUZZER = 1;
      printf("\ncounter: %d \n", temp);
      showADC(ADC_LIGHT);
      showADC(ADC_TEMP);
      
      /*
      printf("seconds: %02x, minutes: %02x, hour: %02x, day: %02x, month: %02x, weekday: %02x, year: %02x\n", 
          ds_readbyte(DS_ADDR_SECONDS),  ds_readbyte(DS_ADDR_MINUTES),  ds_readbyte(DS_ADDR_HOUR),
          ds_readbyte(DS_ADDR_DAY),  ds_readbyte(DS_ADDR_MONTH),  ds_readbyte(DS_ADDR_WEEKDAY), ds_readbyte(DS_ADDR_YEAR));
      printf("DS_ADDR_TCSDS: %02x\n", ds_readbyte(DS_ADDR_TCSDS));
      */
      
      
      ds_readburst((uint8_t *) &rtc);
      printf("yy mm dd hh mm ss am/pm 24/12 ww \n%d%d %d%d %d%d %d%d %d%d %d%d     %d     %d  %d\n",
          rtc.tenyear, rtc.year, rtc.tenmonth, rtc.month, rtc.tenday, rtc.day, rtc.h12.tenhour, rtc.h12.hour, 
          rtc.tenminutes, rtc.minutes, rtc.tenseconds, rtc.seconds, rtc.h12.pm, rtc.h12.hour_12_24, rtc.weekday);
          
      _delay_ms(250);        
      _delay_ms(250);
      _delay_ms(250);
      temp++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
