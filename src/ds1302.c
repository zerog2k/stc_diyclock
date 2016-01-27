// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#include "ds1302.h"

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

void ds_init() {
    uint8_t b = ds_readbyte(DS_ADDR_SECONDS);
    ds_writebyte(DS_ADDR_WP, 0); // clear WP
    b &= ~(0b10000000);
    ds_writebyte(DS_ADDR_SECONDS, b); // clear CH
}

// reset date, time
void ds_reset_clock() {
    ds_writebyte(DS_ADDR_MINUTES, 0x00);
    ds_writebyte(DS_ADDR_HOUR, 0x81);
    ds_writebyte(DS_ADDR_MONTH, 0x01);
    ds_writebyte(DS_ADDR_DAY, 0x01);
}
    
// increment hours
void ds_hours_incr(struct ds1302_rtc* rtc) {
    // TODO: handle 24 hr
    uint8_t hours, b;    
    hours = ds_split2int(rtc->h12.tenhour, rtc->h12.hour);
    if (hours < 12)
        hours++;
    else {
        hours = 1;
        rtc->h12.pm = !rtc->h12.pm;
    }
    b = rtc->h12.hour_12_24 << 7 | rtc->h12.pm << 5 | ds_int2bcd(hours);
    ds_writebyte(DS_ADDR_HOUR, b);
}

// increment minutes
void ds_minutes_incr(struct ds1302_rtc* rtc) {
    uint8_t minutes = ds_split2int(rtc->tenminutes, rtc->minutes);
    if (minutes < 59)
        minutes++;
    else
        minutes = 1;
    ds_writebyte(DS_ADDR_MINUTES, ds_int2bcd(minutes));
}

// increment month
void ds_month_incr(struct ds1302_rtc* rtc) {
    uint8_t month = ds_split2int(rtc->tenmonth, rtc->month);
    if (month < 12)
        month++;
    else
        month = 1;
    ds_writebyte(DS_ADDR_MONTH, ds_int2bcd(month));
}

// increment day
void ds_day_incr(struct ds1302_rtc* rtc) {
    uint8_t day = ds_split2int(rtc->tenday, rtc->day);
    if (day < 31)
        day++;
    else
        day = 1;
    ds_writebyte(DS_ADDR_DAY, ds_int2bcd(day));
}

void ds_weekday_incr(struct ds1302_rtc* rtc) {
    if (rtc->weekday < 7)
        rtc->weekday++;
    else
        rtc->weekday = 1;
    ds_writebyte(DS_ADDR_WEEKDAY, rtc->weekday);
}
    
uint8_t ds_split2int(uint8_t tens, uint8_t ones) {
    return tens * 10 + ones;
}

// return bcd byte from integer
uint8_t ds_int2bcd(uint8_t integer) {
    return integer / 10 << 4 | integer % 10;
}

uint8_t ds_int2bcd_tens(uint8_t integer) {
    return integer / 10;
}

uint8_t ds_int2bcd_ones(uint8_t integer) {
    return integer % 10;
}
