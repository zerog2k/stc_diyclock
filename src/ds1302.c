// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#pragma callee_saves _sendbyte,_readbyte
#pragma callee_saves ds_writebyte,ds_readbyte

#include "ds1302.h"

#define MAGIC_HI  0x5A
#define MAGIC_LO  0xA5

ds1302_rtc_t rtc;
ram_config_t config;

#define INCR(num, low, high) { if (num < high) { num++; } else { num = low; } }

void ds_ram_config_init() {
    uint8_t i,j;
    // check magic bytes to see if ram has been written before
    if ( (ds_readbyte( DS_CMD_RAM >> 1 | 0x00) != MAGIC_LO || ds_readbyte( DS_CMD_RAM >> 1 | 0x01) != MAGIC_HI) ) {
        // if not, must init ram config to defaults
        ds_writebyte( DS_CMD_RAM >> 1 | 0x00, MAGIC_LO);
        ds_writebyte( DS_CMD_RAM >> 1 | 0x01, MAGIC_HI);

        ds_ram_config_write();	// OPTIMISE : Will generate a ljmp to ds_ram_config_write
        return;
    }
    
    // read ram config
    // OPTIMISE : end condition of loop !=4 will generate less code than <4 
    // OPTIMISE : was cfg_table[i] = ds_readbyte(DS_CMD_RAM >> 1 | (i+2));
    //            using a second variable instead of DS_CMD_RAM>>1 | (i+2) generates less code
    j = DS_CMD_RAM >> 1 | 2;
    for (i = 0; i != sizeof(config); i++) {
        *((uint8_t *) config + i) = ds_readbyte(j++);
    }
}

void ds_ram_config_write() {
    uint8_t i, j;
    j = DS_CMD_RAM >> 1 | 2;
    for (i = 0; i != sizeof(config); i++) {
        ds_writebyte( j++, *((uint8_t *) config + i));
    }
}

void _sendbyte(uint8_t b)
{
    b;
  __asm
    push	ar7
    mov     a,dpl
    mov	r7,#8
00001$:
    nop
    nop
    rrc     a
    mov     _DS_IO,c
    setb	_DS_SCLK
    nop
    nop
    clr	_DS_SCLK
    djnz	r7,00001$
    pop	ar7
  __endasm;
}

uint8_t _readbyte()
{
  __asm
	push	ar7
	mov 	a,#0
	mov 	r7,#8
00002$:
	nop
	nop
	mov	c,_DS_IO
	rrc	a	
	setb	_DS_SCLK
	nop
	nop
	clr	_DS_SCLK
	djnz	r7,00002$
	mov	dpl,a
	pop	ar7
  __endasm;
}

uint8_t ds_readbyte(uint8_t addr) {
    // ds1302 single-byte read
    uint8_t b, cmd;
    cmd = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    _sendbyte(cmd);
    // read byte
    b = _readbyte();
    DS_CE = 0;
    return b;
}

void ds_readburst() {
    // ds1302 burst-read 8 bytes into struct
    uint8_t j, b, cmd;
    uint8_t *p_rtc = (uint8_t *) rtc;
    cmd = DS_CMD | DS_CMD_CLOCK | DS_BURST_MODE << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    _sendbyte(cmd);
    // read bytes
    for (j = 0; j != 7; j++) {
        *(p_rtc++) = _readbyte();
    }

    DS_CE = 0;
}

void ds_writebyte(uint8_t addr, uint8_t data) {
    // ds1302 single-byte write
    uint8_t b;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_WRITE;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    _sendbyte(b);
    // send data byte
    _sendbyte(data);

    DS_CE = 0;
}

void ds_init() {
    uint8_t b = ds_readbyte(DS_ADDR_SECONDS);
    ds_writebyte(DS_ADDR_WP, 0); // clear WP
    b &= ~(0b10000000);
    ds_writebyte(DS_ADDR_SECONDS, b); // clear CH
}

/*
// reset date, time
void ds_reset_clock() {
    ds_writebyte(DS_ADDR_MINUTES, 0x00);
    ds_writebyte(DS_ADDR_HOUR,  DS_MASK_1224_MODE|0x07);
    ds_writebyte(DS_ADDR_MONTH, 0x01);
    ds_writebyte(DS_ADDR_DAY,   0x01);
}
*/

void ds_hours_12_24_toggle() {

    uint8_t hours = rtc.h12.hour;  // just the ones place

    if (rtc.h12.hour_12_24) {
        // 12 -> 24
        hours += rtc.h12.tenhour * 10; // get tens place
        if (rtc.h12.pm) {
            if (hours < 12)
                hours += 12;
        } else {
            // 12 am -> 00
            if (hours == 12)
                hours = 0;
        }
        rtc.h24.tenhour = hours / 10;
    } else {
        // 24 -> 12
        hours += rtc.h24.tenhour * 10; // get tens place
        if (hours < 13)
        {
            // 00 -> 12 am
            if (hours == 0)
               hours = 12;
            // 00 - 12 hrs: am
            rtc.h12.pm = 0;
        } else {
            // 13 - 23 hrs: pm
            hours -= 12;
            rtc.h12.pm = 1;
        }
        rtc.h12.tenhour = hours / 10;          
    }
    rtc.h12.hour = hours % 10;
    rtc.h12.hour_12_24 = !rtc.h12.hour_12_24;
    ds_sync_rtc_byte(DS_ADDR_HOUR);
}

// increment hours
void ds_hours_incr() {
    uint8_t hours = rtc.h12.hour; // just ones place
    if (rtc.h12.hour_12_24 == HOUR_24) {
        hours += rtc.h24.tenhour * 10; // get tens place
        if (hours < 23)
            hours++;
        else {
            hours = 00;
        }
        rtc.h24.tenhour = hours / 10;
    } else {
        hours += rtc.h12.tenhour * 10; // get tens place
        if (hours < 12)
            hours++;
        else {
            hours = 1;
            rtc.h12.pm = !rtc.h12.pm;
        }
        rtc.h12.tenhour = hours / 10;       
    }
    rtc.h12.hour = hours % 10;
    ds_sync_rtc_byte(DS_ADDR_HOUR);
}

// increment minutes
void ds_minutes_incr() {
    uint8_t minutes = ds_splitbcd2int(rtc.tenminutes, rtc.minutes);
    INCR(minutes, 0, 59);
    rtc.tenminutes = minutes / 10;
    rtc.minutes = minutes % 10;
    ds_sync_rtc_byte(DS_ADDR_MINUTES);
}

// increment month
void ds_month_incr() {
    uint8_t month = ds_splitbcd2int(rtc.tenmonth, rtc.month);
    INCR(month, 1, 12);
    rtc.tenmonth = month / 10;
    rtc.month = month % 10;
    ds_sync_rtc_byte(DS_ADDR_MONTH);
}

// increment day
void ds_day_incr() {
    uint8_t day = ds_splitbcd2int(rtc.tenday, rtc.day);
    INCR(day, 1, 31);
    rtc.tenday = day / 10;
    rtc.day = day % 10;
    ds_sync_rtc_byte(DS_ADDR_DAY);
}

void ds_alarm_minutes_incr() {
    INCR(config.alarm_minute, 0, 59);
    ds_ram_config_write();
}

void ds_alarm_hours_incr() {
    INCR(config.alarm_hour, 0, 23);
    ds_ram_config_write();
}

void ds_alarm_on_toggle() {
    config.alarm_on = !config.alarm_on;
    ds_ram_config_write();
}

void ds_date_mmdd_toggle() {
    config.show_mmdd = !config.show_mmdd;
    ds_ram_config_write();
}

void ds_temperature_offset_incr() {
    INCR(config.temp_offset, -6, 6);
    ds_ram_config_write();
}

void ds_temperature_cf_toggle() {
    config.temp_C_F = !config.temp_C_F;
    ds_ram_config_write();
}

void ds_weekday_incr() {
    rtc.weekday++;
    ds_sync_rtc_byte(DS_ADDR_WEEKDAY);
}

void ds_sec_zero() {
    ds_writebyte(DS_ADDR_SECONDS, 0);
}
    
uint8_t ds_bcd2int(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t ds_splitbcd2int(uint8_t tens, uint8_t ones) {
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

void ds_sync_rtc_byte(uint8_t addr)
{
    uint8_t *p_rtc = (uint8_t *) rtc + addr;
    ds_writebyte(addr, *p_rtc);
}
