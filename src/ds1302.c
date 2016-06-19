// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#pragma callee_saves ds_writebit,ds_readbit
#pragma callee_saves ds_writebyte,ds_readbyte

#include "ds1302.h"

#define MAGIC_HI  0x5A
#define MAGIC_LO  0xA5


void ds_ram_config_init(uint8_t config[4]) {
    uint8_t i;
    // check magic bytes to see if ram has been written before
    if ( (ds_readbyte( DS_CMD_RAM >> 1 | 0x00) != MAGIC_LO || ds_readbyte( DS_CMD_RAM >> 1 | 0x01) != MAGIC_HI) ) {
        // if not, must init ram config to defaults
        ds_writebyte( DS_CMD_RAM >> 1 | 0x00, MAGIC_LO);
        ds_writebyte( DS_CMD_RAM >> 1 | 0x01, MAGIC_HI);

        //for (i=0; i<4; i++)
            //ds_writebyte( DS_CMD_RAM >> 1 | (i+2), 0x00);
	ds_ram_config_write(config);
	return;
    }
    
    // read ram config
    for (i=0; i<4; i++)
        config[i] = ds_readbyte(DS_CMD_RAM >> 1 | (i+2));
}

void ds_ram_config_write(uint8_t config[4]) {
    uint8_t i;
    for (i=0; i<4; i++)
        ds_writebyte( DS_CMD_RAM >> 1 | (i+2), config[i]);
}

void ds_writebit(__bit b) {
    _nop_; _nop_;
    DS_IO = b;
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
    uint8_t i, b;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    for (i=0; i < 8; i++) {
        ds_writebit(b & 0x01);
        _nop_; _nop_;
        b>>=1;
    }
    // read byte
    b=0;
    for (i=0; i < 8; i++) {
	b>>=1;
        if (ds_readbit()) 
            b |= 0x80; // set
        _nop_; _nop_;              
    }
    DS_CE = 0;
    return b;
}

void ds_readburst(uint8_t time[8]) {
    // ds1302 burst-read 8 bytes into struct
    uint8_t i, j, b;
    b = DS_CMD | DS_CMD_CLOCK | DS_BURST_MODE << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    for (i=0; i < 8; i++) {
        ds_writebit(b & 0x01);
        _nop_; _nop_;
	b>>=1;
    }
    // read bytes
    for (j=0; j < 8; j++) {
	b=0;
        for (i=0; i < 8; i++) {
	    b>>=1;
            if (ds_readbit()) 
                b |= 0x80; // set
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

void ds_init() {
    uint8_t b = ds_readbyte(DS_ADDR_SECONDS);
    ds_writebyte(DS_ADDR_WP, 0); // clear WP
    b &= ~(0b10000000);
    ds_writebyte(DS_ADDR_SECONDS, b); // clear CH
}

// reset date, time
void ds_reset_clock() {
    ds_writebyte(DS_ADDR_MINUTES, 0x00);
    ds_writebyte(DS_ADDR_HOUR, 0x87);
    ds_writebyte(DS_ADDR_MONTH, 0x01);
    ds_writebyte(DS_ADDR_DAY, 0x01);
}
    
void ds_hours_12_24_toggle() {
    uint8_t b = 0;
    b = (! rtc.h24.hour_12_24) << 7;    // toggle 12/24 bit
    ds_writebyte(DS_ADDR_HOUR, b);
}

// increment hours
void ds_hours_incr() {
    uint8_t hours, b = 0;
    if (rtc.h24.hour_12_24 == HOUR_24) {
        hours = ds_split2int(rtc.h24.tenhour, rtc.h24.hour);
        if (hours < 23)
            hours++;
        else {
            hours = 00;
        }
        b = rtc.h24.hour_12_24 << 7 | ds_int2bcd(hours);
    } else {
        hours = ds_split2int(rtc.h12.tenhour, rtc.h12.hour);
        if (hours < 12)
            hours++;
        else {
            hours = 1;
            rtc.h12.pm = !rtc.h12.pm;
        }
        b = rtc.h12.hour_12_24 << 7 | rtc.h12.pm << 5 | ds_int2bcd(hours);        
    }
    
    ds_writebyte(DS_ADDR_HOUR, b);
}

// increment minutes
void ds_minutes_incr() {
    uint8_t minutes = ds_split2int(rtc.tenminutes, rtc.minutes);
    if (minutes < 59)
        minutes++;
    else
        minutes = 1;
    ds_writebyte(DS_ADDR_MINUTES, ds_int2bcd(minutes));
}

// increment month
void ds_month_incr() {
    uint8_t month = ds_split2int(rtc.tenmonth, rtc.month);
    if (month < 12)
        month++;
    else
        month = 1;
    ds_writebyte(DS_ADDR_MONTH, ds_int2bcd(month));
}

// increment day
void ds_day_incr() {
    uint8_t day = ds_split2int(rtc.tenday, rtc.day);
    if (day < 31)
        day++;
    else
        day = 1;
    ds_writebyte(DS_ADDR_DAY, ds_int2bcd(day));
}

void ds_weekday_incr() {
    if (rtc.weekday < 7)
        rtc.weekday++;
    else
        rtc.weekday = 1;
    ds_writebyte(DS_ADDR_WEEKDAY, rtc.weekday);
}
    
uint8_t ds_split2int(uint8_t tens, uint8_t ones) {
    return tens * 10 + ones;
}

// return bcd byte from integer
uint8_t ds_int2bcd(uint8_t integer) {
    return integer / 10 << 4 | integer % 10;
}

uint8_t ds_int2bcd_tens(uint8_t integer) {
    return integer / 10 % 10;
}

uint8_t ds_int2bcd_ones(uint8_t integer) {
    return integer % 10;
}
