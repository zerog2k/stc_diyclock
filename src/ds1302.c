// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#pragma callee_saves sendbyte,readbyte
#pragma callee_saves ds_writebyte,ds_readbyte

#include "ds1302.h"

#define MAGIC_HI  0x5A
#define MAGIC_LO  0xA5

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
    // OPTIMISE : was config_table[i] = ds_readbyte(DS_CMD_RAM >> 1 | (i+2));
    //            using a second variable instead of DS_CMD_RAM>>1 | (i+2) generates less code
    j=DS_CMD_RAM>>1|2;
    for (i=0; i!=4; i++)
        config_table[i] = ds_readbyte(j++);
}

void ds_ram_config_write() {
    uint8_t i,j;
    j=DS_CMD_RAM>>1|2;
    for (i=0; i!=4; i++)
        ds_writebyte( j++, config_table[i]);
}

void sendbyte(uint8_t b)
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
        mov     _P1_1,c
	setb	_P1_2
	nop
	nop
	clr	_P1_2
	djnz	r7,00001$
	pop	ar7
  __endasm;
}

uint8_t readbyte()
{
  __asm
	push	ar7
	mov 	a,#0
	mov 	r7,#8
00002$:
	nop
	nop
	mov	c,_P1_1
	rrc	a	
	setb	_P1_2
	nop
	nop
	clr	_P1_2
	djnz	r7,00002$
	mov	dpl,a
	pop	ar7
  __endasm;
}

uint8_t ds_readbyte(uint8_t addr) {
    // ds1302 single-byte read
    uint8_t b;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    sendbyte(b);
    // read byte
    b=readbyte();
    DS_CE = 0;
    return b;
}

void ds_readburst() {
    // ds1302 burst-read 8 bytes into struct
    uint8_t j, b;
    b = DS_CMD | DS_CMD_CLOCK | DS_BURST_MODE << 1 | DS_CMD_READ;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    sendbyte(b);
    // read bytes
    for (j=0; j!=8; j++) 
        rtc_table[j] = readbyte();
    DS_CE = 0;
}

void ds_writebyte(uint8_t addr, uint8_t data) {
    // ds1302 single-byte write
    uint8_t b = 0;
    b = DS_CMD | DS_CMD_CLOCK | addr << 1 | DS_CMD_WRITE;
    DS_CE = 0;
    DS_SCLK = 0;
    DS_CE = 1;
    // send cmd byte
    sendbyte(b);
    // send data byte
    sendbyte(data);

    DS_CE = 0;
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
    ds_writebyte(DS_ADDR_HOUR,  DS_MASK_AMPM_MODE|0x07);
    ds_writebyte(DS_ADDR_MONTH, 0x01);
    ds_writebyte(DS_ADDR_DAY,   0x01);
}
    
void ds_hours_12_24_toggle() {

    uint8_t hours,b;
    if (H12_24)
    { // 12h->24h
      hours=ds_split2int(rtc_table[DS_ADDR_HOUR]&DS_MASK_HOUR12); // hours in 12h format (1-11am 12pm 1-11pm 12am)
      if (hours==12) 
       {if (!H12_PM) hours=0;}
      else
       {if (H12_PM) hours+=12;}			 // to 24h format
      b = ds_int2bcd(hours);			 // clear hour_12_24 bit
    }
    else
    { // 24h->12h 
      hours = ds_split2int(rtc_table[DS_ADDR_HOUR]&DS_MASK_HOUR24); // hours in 24h format (0-23, 0-11=>am , 12-23=>pm)
      b = DS_MASK_AMPM_MODE; 
      if (hours >= 12) { hours-=12; b|=0x20; }	// pm
      if (hours == 0) { hours=12; } 		//12am
      b |= ds_int2bcd(hours);
    }

    ds_writebyte(DS_ADDR_HOUR,b);
}

// increment hours
void ds_hours_incr() {
    uint8_t hours, b = 0;
    if (!H12_24) {
        hours = ds_split2int(rtc_table[DS_ADDR_HOUR]&DS_MASK_HOUR24);	//24h format
        if (hours < 23)
            hours++;
        else {
            hours = 00;
        }
        b = ds_int2bcd(hours);		// bit 7 = 0
    } else {
        hours = ds_split2int(rtc_table[DS_ADDR_HOUR]&DS_MASK_HOUR12);	//12h format
        if (hours < 12)
            hours++;
        else {
            hours = 1;
            H12_PM=!H12_PM;
        }
        b = (H12_PM?(DS_MASK_AMPM_MODE|DS_MASK_PM):DS_MASK_AMPM_MODE) | ds_int2bcd(hours);        
    }
    
    ds_writebyte(DS_ADDR_HOUR, b);
}

// increment minutes
void ds_minutes_incr() {
    uint8_t minutes = ds_split2int(rtc_table[DS_ADDR_MINUTES]&DS_MASK_MINUTES);
    if (minutes < 59)
        minutes++;
    else
        minutes = 1;
    ds_writebyte(DS_ADDR_MINUTES, ds_int2bcd(minutes));
}

// increment month
void ds_month_incr() {
    uint8_t month = ds_split2int(rtc_table[DS_ADDR_MONTH]&DS_MASK_MONTH);
    if (month < 12)
        month++;
    else
        month = 1;
    ds_writebyte(DS_ADDR_MONTH, ds_int2bcd(month));
}

// increment day
void ds_day_incr() {
    uint8_t day = ds_split2int(rtc_table[DS_ADDR_DAY]&DS_MASK_DAY);
    if (day < 31)
        day++;
    else
        day = 1;
    ds_writebyte(DS_ADDR_DAY, ds_int2bcd(day));
}

void ds_weekday_incr() {
    uint8_t day = rtc_table[DS_ADDR_WEEKDAY];
    if (day < 7)
        day++;
    else
        day=1;
    ds_writebyte(DS_ADDR_WEEKDAY, day);
    rtc_table[DS_ADDR_WEEKDAY] = day;		// usefull ?
}

void ds_sec_zero() {
    rtc_table[DS_ADDR_SECONDS]=0;
    ds_writebyte(DS_ADDR_SECONDS,0);
}
    
uint8_t ds_split2int(uint8_t tens_ones) {
    return (tens_ones>>4) * 10 + (tens_ones&0xF);
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
