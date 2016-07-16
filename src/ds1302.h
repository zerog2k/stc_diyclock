// DS1302 RTC IC
// http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
//

#include <stc12.h>
#include <stdint.h>

#define _nop_ __asm nop __endasm;

#define DS_CE    P1_0
#define _DS_CE    _P1_0
#define DS_IO    P1_1
#define _DS_IO    _P1_1
#define DS_SCLK  P1_2
#define _DS_SCLK  _P1_2

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

// DS_ADDR_SECONDS	c111_1111	0_0-5_9 c=clock_halt
// DS_ADDR_MINUTES	x111_1111	0_0-5_9
// DS_ADDR_HOUR		a0b1_1111	0_1-1_2/0_0-2_3 - a=12/not 24, b=not AM/PM if a=1 , else hour(0x20) 
// DS_ADDR_DAY          0011_1111	0_1-3_1
// DS_ADDR_MONTH        0001_1111	0_1-1_2
// DS_ADDR_WEEKDAY      0000_0111	0_1-0_7
// DS_ADDR_YEAR		1111_1111	0_0-9_9 

#define DS_MASK_SECONDS       0b01111111
#define DS_MASK_SECONDS_TENS  0b01110000
#define DS_MASK_SECONDS_UNITS 0b00001111
#define DS_MASK_MINUTES       0b01111111
#define DS_MASK_MINUTES_TENS  0b01110000
#define DS_MASK_MINUTES_UNITS 0b00001111
#define DS_MASK_AMPM_MODE     0b10000000
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

// 8051 zone from 0x20 and 0x2F in IRAM can be accessed as bit (between 0x00 and 0x7F)
// 0x20, bit 0 is __bit __at (0x00), bit 7 is __bit __at (0x07) etc...

uint8_t __at (0x24) rtc_table[8];

// h12.tenhour in RTC is at address 0x26, bit 4 -> => 0x26-0x20 => 0x6*8+4 => 52 => 0x34
__bit __at (0x34) H12_TH;
// h12.pm in RTC is at address 0x26, bit 5 -> => 0x26-0x20 => 0x6*8+5 => 53 => 0x35
__bit __at (0x35) H12_PM;
// hour_12_24 in RTC is at address 0x26, bit 7 -> => 0x26-0x20 => 0x6*8+7 => 55 => 0x37
__bit __at (0x37) H12_24;

// config in DS1302 RAM

uint8_t __at (0x2c) config_table[4];

#define CONFIG_ALARM_HOURS_BYTE   0
#define CONFIG_ALARM_MINUTES_BYTE 1
#define CONFIG_TEMP_BYTE          2
#define CONFIG_CHIME_START_BYTE   2
#define CONFIG_CHIME_STOP_BYTE    3

#define CONFIG_ALARM_HOURS_MASK   0b00011111
#define CONFIG_ALARM_MINUTES_MASK 0b00111111
#define CONFIG_TEMP_MASK          0b11100000
#define CONFIG_TEMP_SHIFT         5
#define CONFIG_CHIME_START_MASK   0b00011111
#define CONFIG_CHIME_STOP_MASK    0b00011111

// Offset 0 => temp_C_F (7) / Alarm_on (6) / Chime_on(5) / alarm_hour (4..0)
// Offset 1 => (7) not used / (6) sw_mmdd / alarm_minute (5..0)
// Offset 2 => temp_offset (7..5) / chime_hour_start (4..0) 
// Offset 3 => (7),(6)&(5) not used / chime_hour_stop (4..0)

// temp_C_F in config is at address 0x2c, bit 0 => 0x2c-0x20 => 0xc*8+0 => 96 => 0x60
__bit __at (0x67) CONF_C_F;
__bit __at (0x66) CONF_ALARM_ON;
__bit __at (0x65) CONF_CHIME_ON;
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
void ds_reset_clock();

// toggle 12/24 hour mode
void ds_hours_12_24_toggle();
    
// increment hours
void ds_hours_incr();

// increment minutes
void ds_minutes_incr();

// increment month
void ds_month_incr();

// increment day
void ds_day_incr();

void ds_weekday_incr();
void ds_sec_zero();
    
// split bcd to int
uint8_t ds_split2int(uint8_t tens_ones);

// return bcd byte from integer
uint8_t ds_int2bcd(uint8_t integer);
    
// convert integer to bcd parts (high = tens, low = ones)
uint8_t ds_int2bcd_tens(uint8_t integer);
uint8_t ds_int2bcd_ones(uint8_t integer);
    
