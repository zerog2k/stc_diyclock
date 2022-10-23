// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#include "stc15.h"
#include <stdint.h>
#include "hwconfig.h"

#define _nop_ __asm nop __endasm;

#define DS_CMD        0x80
#define DS_CMD_READ   1
#define DS_CMD_WRITE  0
#define DS_CMD_RAM    0x40
#define DS_CMD_CLOCK  0x00
#define DS_BURST_MODE 0x3E

#define DS_ADDR_SECONDS     0
#define DS_ADDR_MINUTES     1
#define DS_ADDR_HOUR        2
#define DS_ADDR_DAY         3
#define DS_ADDR_MONTH       4
#define DS_ADDR_WEEKDAY     5
#define DS_ADDR_YEAR        6
#define DS_ADDR_WP          7
#define DS_ADDR_TCSDS       8

// trickle charge for super capacitors - 1/2 diodes, 2/4/8 kOhm resistor
#define DS_TCS_TCON 0b10100000

#define DS_TC_D1_2KO 0b0101
#define DS_TC_D1_4KO 0b0110
#define DS_TC_D1_8KO 0b0111
#define DS_TC_D2_2KO 0b1001
#define DS_TC_D2_4KO 0b1010
#define DS_TC_D2_8KO 0b1011


// DS_ADDR_SECONDS	c111_1111	0_0-5_9 c=clock_halt
// DS_ADDR_MINUTES	x111_1111	0_0-5_9
// DS_ADDR_HOUR		a0b1_1111	0_1-1_2/0_0-2_3 - a=12/not 24, b=not AM/PM if a=1 , else hour(0x20) 
// DS_ADDR_DAY          0011_1111	0_1-3_1
// DS_ADDR_MONTH        0001_1111	0_1-1_2
// DS_ADDR_WEEKDAY      0000_0111	0_1-0_7
// DS_ADDR_YEAR		1111_1111	0_0-9_9 

#define DS_CLOCK_HALT         0b10000000

#define DS_MASK_SECONDS       0b01111111
#define DS_MASK_SECONDS_TENS  0b01110000
#define DS_MASK_SECONDS_UNITS 0b00001111
#define DS_MASK_MINUTES       0b01111111
#define DS_MASK_MINUTES_TENS  0b01110000
#define DS_MASK_MINUTES_UNITS 0b00001111
#define DS_MASK_1224_MODE     0b10000000
#define DS_MASK_PM            0b00100000
#define DS_MASK_HOUR12        0b00011111
#define DS_MASK_HOUR12_TENS   0b00010000
#define DS_MASK_HOUR24        0b00111111
#define DS_MASK_HOUR24_TENS   0b00110000
#define DS_MASK_HOUR_UNITS    0b00001111
#define DS_MASK_DAY           0b00111111
#define DS_MASK_DAY_TENS      0b00110000
#define DS_MASK_DAY_UNITS     0b00001111
#define DS_MASK_MONTH         0b00011111
#define DS_MASK_MONTH_TENS    0b00010000
#define DS_MASK_MONTH_UNITS   0b00001111
#define DS_MASK_WEEKDAY       0b00000111
#define DS_MASK_YEAR          0b11111111
#define DS_MASK_YEAR_TENS     0b11110000
#define DS_MASK_YEAR_UNITS    0b00001111

/* 
  NB: the rtc and config bits below were originally structs/unions, but for some reason 
  the resulting code was bloated coming out of sdcc. This is attempt to recreate this
  struct/union functionality directly, by explicit selection of bit/byte iram addressing
  and using masks above. Because this current implementation is much more complex than
  dealing with structs/unions, would like to someday return to simpler struct/union access
  if code size allows (sdcc optimizations?).
*/

// 8051 zone from 0x20 and 0x2F in IRAM can be accessed as bit (between 0x00 and 0x7F)
// 0x20, bit 0 is __bit __at (0x00), bit 7 is __bit __at (0x07) etc...

uint8_t __at (0x24) rtc_table[8];

// h12.tenhour in RTC is at address 0x26, bit 4 -> => 0x26-0x20 => 0x6*8+4 => 52 => 0x34
__bit __at (0x34) H12_TH;
// h12.pm in RTC is at address 0x26, bit 5 -> => 0x26-0x20 => 0x6*8+5 => 53 => 0x35
__bit __at (0x35) H12_PM;
// hour_12_24 in RTC is at address 0x26, bit 7 -> => 0x26-0x20 => 0x6*8+7 => 55 => 0x37
__bit __at (0x37) H12_12;

// config in DS1302 RAM

uint8_t __at (0x2c) cfg_table[4];

#define CFG_ALARM_HOURS_BYTE   0
#define CFG_ALARM_HOURS_MASK   0b11111000
#define CFG_ALARM_HOURS_SHIFT  3

#define CFG_ALARM_MINUTES_BYTE 1
#define CFG_ALARM_MINUTES_MASK 0b00111111

#define CFG_TEMP_BYTE          2
#define CFG_TEMP_MASK          0b00000111

#define CFG_CHIME_SINCE_BYTE   2
#define CFG_CHIME_SINCE_MASK   0b11111000
#define CFG_CHIME_SINCE_SHIFT  3

#define CFG_CHIME_UNTIL_BYTE   3
#define CFG_CHIME_UNTIL_MASK   0b00011111

// Offset 0 => alarm_hour (7..3) / chime_on (2) / alarm_on (1) / temp_C_F (0)
// Offset 1 => (7) not used / (6) sw_mmdd / alarm_minute (5..0)
// Offset 2 => chime_hour_start (7..3) / temp_offset (2..0), signed -4 / +3
// Offset 3 => (7),(6)&(5) not used / chime_hour_stop (4..0)

// temp_C_F in config is at address 0x2c, bit 0 => 0x2c-0x20 => 0xc*8+0 => 96 => 0x60
__bit __at (0x60) CONF_C_F;
__bit __at (0x61) CONF_ALARM_ON;
__bit __at (0x62) CONF_CHIME_ON;
__bit __at (0x6E) CONF_SW_MMDD;

// DS1302 Functions

void ds_ram_config_init();
void ds_ram_config_write();

// ds1302 single-byte read
uint8_t ds_readbyte(uint8_t addr);

// ds1302 burst-read 8 bytes into struct
void ds_readburst();

// ds1302 single-byte write
void ds_writebyte(uint8_t addr, uint8_t data);

// clear WP, CH
void ds_init();

// reset date/time to 01/01 00:00
//void ds_reset_clock();

// toggle 12/24 hour mode
void ds_hours_12_24_toggle();
    
// increment hours
void ds_hours_incr();

// increment minutes
void ds_minutes_incr();

// increment year
void ds_year_incr();

// increment month
void ds_month_incr();

// increment day
void ds_day_incr();

void ds_weekday_incr();
void ds_sec_zero();
    
// split bcd to int
uint8_t ds_bcd2int(uint8_t tens_ones);

// return bcd byte from integer
uint8_t ds_int2bcd(uint8_t integer);
    
// convert integer to bcd parts (high = tens, low = ones)
uint8_t ds_int2bcd_tens(uint8_t integer);
uint8_t ds_int2bcd_ones(uint8_t integer);

void ds_alarm_minutes_incr();
void ds_alarm_hours_incr();
void ds_alarm_on_toggle();
void ds_chime_since_incr();
void ds_chime_until_incr();
void ds_chime_on_toggle();
void ds_date_mmdd_toggle();
void ds_temperature_offset_incr();
void ds_temperature_cf_toggle();
