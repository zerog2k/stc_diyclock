// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#include <stc12.h>
#include <stdint.h>

#define _nop_ __asm nop __endasm;

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

typedef struct ds1302_rtc {
    // inspiration from http://playground.arduino.cc/Main/DS1302
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
            uint8_t reserved2a:1;
            uint8_t hour_12_24:1;    // 0=24h
        } h24;
        struct {
            // 0-23
            uint8_t hour:4;      
            uint8_t tenhour:1;
            uint8_t pm:1;           // 0=am, 1=pm;
            uint8_t reserved2b:1;
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


void ds_writebit(__bit bit);

__bit ds_readbit();

// ds1302 single-byte read
uint8_t ds_readbyte(uint8_t addr);

// ds1302 burst-read 8 bytes into struct
void ds_readburst(uint8_t time[8]);

// ds1302 single-byte write
uint8_t ds_writebyte(uint8_t addr, uint8_t data);

// clear WP, CH
void ds_init();

// reset date/time to 01/01 00:00
void ds_reset_clock();
    
// set hours
void ds_set_hours(struct ds1302_rtc* rtc);

// set minutes
void ds_set_minutes(struct ds1302_rtc* rtc);

// set month
void ds_set_month(struct ds1302_rtc* rtc);

// set day
void ds_set_day(struct ds1302_rtc* rtc);

// increment hours
void ds_hours_incr(struct ds1302_rtc* rtc);

// increment minutes
void ds_minutes_incr(struct ds1302_rtc* rtc);

// increment month
void ds_month_incr(struct ds1302_rtc* rtc);

// increment day
void ds_day_incr(struct ds1302_rtc* rtc);

// split bcd to int
uint8_t ds_split2int(uint8_t tens, uint8_t ones);

// int to bcd
uint8_t ds_int2bcd(uint8_t integer);
    