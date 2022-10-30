// DS1302 RTC IC -- see http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
#include "ds1302.h"

#pragma callee_saves sendbyte, readbyte
#pragma callee_saves ds_writebyte, ds_readbyte
// silence these warnings:
// src/ds1302.c:101: warning 59: function 'readbyte' must return value
// src/ds1302.c:398: warning 59: function 'ds_int2bcd' must return value
#pragma disable_warning 59

#define MAGIC_HI 0x5A
#define MAGIC_LO 0xA5

#define INCR(num, low, high) if(num<high){num++;}else{num=low;}

/*
  Judge whether need to initialize RAM or not by checking RAM address 0x10-0x11 has A5,5A (MAGIC).
  If the MAGIC key is found, read the data. Otherwise initialize.
 */
void ds_ram_config_init() {
  uint8_t i, j;
  // check magic bytes to see if ram has been written before
  if ((ds_readbyte(DS_CMD_RAM >> 1 | 0x00) != MAGIC_LO || ds_readbyte(DS_CMD_RAM >> 1 | 0x01) != MAGIC_HI)) {
    // if not, must init ram config to defaults
    ds_writebyte(DS_CMD_RAM >> 1 | 0x00, MAGIC_LO);
    ds_writebyte(DS_CMD_RAM >> 1 | 0x01, MAGIC_HI);

    ds_ram_config_write();  // OPTIMIZATION: Will generate a ljmp to ds_ram_config_write
    return;
  }

  // read ram config
  // OPTIMIZATION: end condition of loop !=4 will generate less code than <4
  // OPTIMIZATION: was cfg_table[i] = ds_readbyte(DS_CMD_RAM >> 1 | (i+2));
  //               using a second variable instead of DS_CMD_RAM>>1 | (i+2) generates less code
  j = DS_CMD_RAM >> 1 | 2;
  for (i = 0; i != 4; i++) {
    cfg_table[i] = ds_readbyte(j);
    j++;
  }
}

/*
  Data written to EEPROM
  0x10 A5             (written by ds_ram_config_init)
  0x11 5A             (written by ds_ram_config_init)
  0x12 HH             <- cfg_table[0]
  0x13 MM             <- cfg_table[1]
  0x14 Temperature    <- cfg_table[2]
  0x15 --             <- cfg_table[3]
 */
void ds_ram_config_write() {
  uint8_t i, j;
  j = DS_CMD_RAM >> 1 | 2;
  for (i = 0; i != 4; i++) {
    ds_writebyte(j, cfg_table[i]);
    j++;
  }
}

void sendbyte(uint8_t b) {
  b; // suppress 'unused parameter' warning
  __asm;
  push ar7;
  mov a, dpl;
  mov r7, #8;
00001$:
  nop;
  nop;
  rrc a;
  mov _DS_IO, c;
  setb _DS_SCLK;
  nop;
  nop;
  clr _DS_SCLK;
  djnz r7, 00001$;
  pop ar7;
  __endasm;
}

uint8_t readbyte() {
  __asm;
  push ar7;
  mov a, #0;
  mov r7, #8;
00002$:
  nop;
  nop;
  mov c, _DS_IO;
  rrc a;
  setb _DS_SCLK;
  nop;
  nop;
  clr _DS_SCLK;
  djnz r7, 00002$;
  mov dpl, a;
  pop ar7;
  __endasm;
}

void ds_writebyte(uint8_t addr, uint8_t data) {
  // ds1302 single-byte write
  uint8_t cmd = DS_CMD | DS_CMD_CLOCK | (addr << 1) | DS_CMD_WRITE;
  DS_CE = 0;
  DS_SCLK = 0;
  DS_CE = 1;
  sendbyte(cmd);
  sendbyte(data);
  DS_CE = 0;
}

uint8_t ds_readbyte(uint8_t addr) {
  // ds1302 single-byte read
  uint8_t b;
  b = DS_CMD | DS_CMD_CLOCK | (addr << 1) | DS_CMD_READ;
  DS_CE = 0;
  DS_SCLK = 0;
  DS_CE = 1;
  // send cmd byte
  sendbyte(b);
  // read byte
  b = readbyte();
  DS_CE = 0;
  return b;
}

void ds_readburst() {
  // ds1302 burst-read 8 bytes into struct
  uint8_t j;
  uint8_t cmd = DS_CMD | DS_CMD_CLOCK | DS_BURST_MODE | DS_CMD_READ;
  DS_CE = 0;
  DS_SCLK = 0;
  DS_CE = 1;
  sendbyte(cmd);
  for (j = 0; j != 8; j++) {
    rtc_table[j] = readbyte();
  }
  DS_CE = 0;
}

/*
  Initialize DS1302 Real-Time Clock

  Control register bit 7 is WP (Write Protect).
  WP=0: all registers can be set
  WP=1: no register can be written
  This bit is unstable after power up, therefore we need to clear it.

  The clock halt (CH) flag is in bit 7 of the SECONDS register.
  CH=1: stop clock oscillator and enter low power mode (under 100nA).
  CH=0: start clock.
  Just after power-up, this bit is unstable, need to initialize.
  But we need to keep the original SECONDS value!
 */
void ds_init() {
  uint8_t b = ds_readbyte(DS_ADDR_SECONDS);
  ds_writebyte(DS_ADDR_WP, 0);       // clear write protect
  b &= 0x7F;                         // clear CH flag (bit7)
  ds_writebyte(DS_ADDR_SECONDS, b);  // start clock
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
  uint8_t hours, b;
  if (H12_12) {                                                    // 12h->24h
    hours = ds_bcd2int(rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR12);  // hours in 12h format (1-11am 12pm 1-11pm 12am)
    if (hours == 12) {
      if (!H12_PM) {
        hours = 0;
      }
    } else {
      if (H12_PM) {
        hours += 12;  // to 24h format
      }
    }
    b = ds_int2bcd(hours);                                         // clear hour_12_24 bit
  } else {                                                         // 24h->12h
    hours = ds_bcd2int(rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR24);  // hours in 24h format (0-23, 0-11=>am , 12-23=>pm)
    b = DS_MASK_1224_MODE;
    if (hours >= 12) {  // pm
      hours -= 12;
      b |= DS_MASK_PM;
    }
    if (hours == 0) {  // 12am
      hours = 12;
    }
    b |= ds_int2bcd(hours);
  }

  ds_writebyte(DS_ADDR_HOUR, b);
}

// NB: this is shorter than with a macro and a 'pure' inc bcd routine
uint8_t inc_bcd(uint8_t val, uint8_t min, uint8_t max) {
  if (val >= max) {
    return min;
  } 
  __asm;
  mov a, r7;
  // NB: "inc a" gives a false result, it does not set the 'auxiliary carry' flag needed by "da a"
  add a, #1;
  da a;
  mov dpl, a;
  __endasm;
}

// increment hours
void ds_hours_incr() {
  uint8_t hours, b = 0;
  if (H12_12) {
    hours = ds_bcd2int(rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR12);  // 12h format
    INCR(hours, 1, 12);
    if (hours == 12) {
      H12_PM = !H12_PM;
    }
    b = ds_int2bcd(hours) | DS_MASK_1224_MODE;  // bit 7 = 1 -> 12h mode
    if (H12_PM) {
      b |= DS_MASK_PM;
    }
  } else {
    hours = (rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR24);  // 24h format
    b = inc_bcd(hours, 0x00, 0x23);  // bit 7 = 0 -> 24h mode
  }
  ds_writebyte(DS_ADDR_HOUR, b);
}

// increment minutes
void ds_minutes_incr() {
  uint8_t minutes = (rtc_table[DS_ADDR_MINUTES] & DS_MASK_MINUTES);
  minutes = inc_bcd(minutes, 0x00, 0x59);
  ds_writebyte(DS_ADDR_MINUTES, minutes);
}

// increment year
void ds_year_incr() {
  uint8_t year = (rtc_table[DS_ADDR_YEAR] & DS_MASK_YEAR);
  year = inc_bcd(year, 0x00, 0x99);
  ds_writebyte(DS_ADDR_YEAR, year);
}

// increment month
void ds_month_incr() {
  uint8_t month = (rtc_table[DS_ADDR_MONTH] & DS_MASK_MONTH);
  month = inc_bcd(month, 0x01, 0x12);
  ds_writebyte(DS_ADDR_MONTH, month);
}

// increment day
void ds_day_incr() {
  uint8_t day = (rtc_table[DS_ADDR_DAY] & DS_MASK_DAY);
  day = inc_bcd(day, 0x01, 0x31);
  ds_writebyte(DS_ADDR_DAY, day);
}

void ds_alarm_minutes_incr() {
  uint8_t mm = cfg_table[CFG_ALARM_MINUTES_BYTE] & CFG_ALARM_MINUTES_MASK;
  INCR(mm, 0, 59);
  cfg_table[CFG_ALARM_MINUTES_BYTE] &= ~CFG_ALARM_MINUTES_MASK;
  cfg_table[CFG_ALARM_MINUTES_BYTE] |= mm;
  ds_ram_config_write();
}

void ds_alarm_hours_incr() {
  uint8_t hh = cfg_table[CFG_ALARM_HOURS_BYTE] >> CFG_ALARM_HOURS_SHIFT;
  INCR(hh, 0, 23);
  hh <<= CFG_ALARM_HOURS_SHIFT;
  cfg_table[CFG_ALARM_HOURS_BYTE] &= ~CFG_ALARM_HOURS_MASK;
  cfg_table[CFG_ALARM_HOURS_BYTE] |= hh;
  ds_ram_config_write();
}

void ds_alarm_on_toggle() {
  CONF_ALARM_ON = !CONF_ALARM_ON;
  ds_ram_config_write();
}

void ds_chime_start_incr() {
  uint8_t hh = cfg_table[CFG_CHIME_SINCE_BYTE] >> CFG_CHIME_SINCE_SHIFT;
  INCR(hh, 0, 23);
  hh <<= CFG_CHIME_SINCE_SHIFT;
  cfg_table[CFG_CHIME_SINCE_BYTE] &= ~CFG_CHIME_SINCE_MASK;
  cfg_table[CFG_CHIME_SINCE_BYTE] |= hh;
  ds_ram_config_write();
}

void ds_chime_until_incr() {
  uint8_t hh = cfg_table[CFG_CHIME_UNTIL_BYTE] & CFG_CHIME_UNTIL_MASK;
  INCR(hh, 0, 23);
  cfg_table[CFG_CHIME_UNTIL_BYTE] &= ~CFG_CHIME_UNTIL_MASK;
  cfg_table[CFG_CHIME_UNTIL_BYTE] |= hh;
  ds_ram_config_write();
}

void ds_chime_on_toggle() {
  CONF_CHIME_ON = !CONF_CHIME_ON;
  ds_ram_config_write();
}

void ds_date_mmdd_toggle() {
  CONF_SW_MMDD = !CONF_SW_MMDD;
  ds_ram_config_write();
}

void ds_temperature_offset_incr() {
  uint8_t offset = cfg_table[CFG_TEMP_BYTE] & CFG_TEMP_MASK;
  offset++;
  offset &= CFG_TEMP_MASK;
  cfg_table[CFG_TEMP_BYTE] = (cfg_table[CFG_TEMP_BYTE] & ~CFG_TEMP_MASK) | offset;
  ds_ram_config_write();
}

void ds_temperature_cf_toggle() {
  CONF_C_F = !CONF_C_F;
  ds_ram_config_write();
}

void ds_weekday_incr() {
  uint8_t day = rtc_table[DS_ADDR_WEEKDAY];
  INCR(day, 1, 7);
  ds_writebyte(DS_ADDR_WEEKDAY, day);
  rtc_table[DS_ADDR_WEEKDAY] = day;  // useful?
}

void ds_sec_zero() {
  rtc_table[DS_ADDR_SECONDS] = 0;
  ds_writebyte(DS_ADDR_SECONDS, DS_CLOCK_HALT);  // set CH, stop clock
  ds_writebyte(DS_ADDR_SECONDS, 0);              // clear CH, start clock, set seconds to zero
}

// return integer from bcd byte
uint8_t ds_bcd2int(uint8_t tens_ones) {
  // 8051 has 'mul ab', so the generated assembly code is quite short
  return (tens_ones >> 4) * 10 + (tens_ones & 0x0F);
/*
_ds_bcd2int:
;	src/ds1302.c:339: return (tens_ones >> 4) * ((uint8_t)10) + (tens_ones & 0x0F);
	mov	a,dpl
	mov	r7,a
	swap	a
	anl	a,#0x0f
	mov	b,#0x0a
	mul	ab
	mov	r6,a
	anl	ar7,#0x0f
	mov	a,r7
	add	a,r6
	mov	dpl,a
;	src/ds1302.c:340: }
	ret
*/
}

// return bcd byte from integer
uint8_t ds_int2bcd(uint8_t integer) {
#ifdef INT2BCD_ORIG
  // The original uses library calls for div and mod:
  return integer / 10 << 4 | integer % 10;
#elif  INT2BCD_SMALL
  // 8051 has 'div ab', integer result of a/b in a, and remainder in b.
  // If we make sure SDCC knows that all numbers are 8-bit, it's using it:
  uint8_t ten = 10;
  return integer / ten << 4 | integer % ten;
  /*
  Generated code:
  _ds_int2bcd:
	mov	r7,dpl
	mov	b,#0x0a
	mov	a,r7
	div	ab
	swap	a
	anl	a,#0xf0
	mov	r6,a
	mov	b,#0x0a
	mov	a,r7
	div	ab
	mov	a,b
	orl	a,r6
	mov	dpl,a
	ret
  */
#else // smallest variant
  integer; // suppress 'unused parameter' warning
  __asm;
	mov	a,dpl;
	mov	b,#0x0a;
	div	ab;
	swap a;
	anl	a,#0xf0;
  orl a,b;
	mov	dpl,a;
	ret;
   __endasm;
#endif
}
