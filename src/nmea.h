#include "eeprom.h"
#include <time.h>

#define BAUDRATE 9600 // serial port speed (4800/9600 - standard for GPS)

#define MIN_NMEA_PAUSE 21600 // min pause between syncs (sec) - 6hr

volatile uint32_t sync_remaining = 0;

#define IAP_TZ_ADDRESS 0x0000
#define IAP_TZ_HR      0x0000
#define IAP_TZ_MIN     0x0001
#define IAP_TZ_DST     0x0002

volatile int8_t nmea_tz_hr;
volatile uint8_t nmea_tz_min;
volatile uint8_t nmea_tz_dst;
int8_t nmea_prev_tz_hr;
uint8_t nmea_prev_tz_min;
uint8_t nmea_prev_tz_dst;

#define NMEA_LINE_LEN_MAX 84

__pdata char ubuf[NMEA_LINE_LEN_MAX];
volatile int8_t uidx = 0;

__code const char NMEA_TOKEN[] = "$GPRMC";

volatile enum {
    NMEA_NONE = 0,
    NMEA_START,
    NMEA_PARSE,
    NMEA_SET
} nmea_state = NMEA_NONE;

void uart1_init()
{
    P_SW1 |= (1 << 6);          // move UART1 pins -> P3_6:rxd, P3_7:txd
    // UART1 use Timer2
    T2L = (65536 - (FOSC / 4 / BAUDRATE)) & 0xFF;
    T2H = (65536 - (FOSC / 4 / BAUDRATE)) >> 8;
    SM1 = 1;                    // serial mode 1: 8-bit async
    AUXR |= 0x14;               // T2R: run T2, T2x12: T2 clk src sysclk/1
    AUXR |= 0x01;               // S1ST2: T2 is baudrate generator
    ES = 1;                     // enable uart1 interrupt
    EA = 1;                     // enable interrupts
    REN = 1;
}

// return pointer to comma N num in ubuf
char *nmea_comma(uint8_t num)
{
    int8_t i;
    for (i = 0; i < uidx; i++)
        if (ubuf[i] == ',' && !--num)
            return ubuf + i;
    return NULL;
}

// just simple memcmp, ret 0 == equal
int8_t nmea_cmp(char *dst, char *src, uint8_t n)
{
    while (n--)
        if (*dst++ != *src++)
            return 1;
    return 0;
}

// just simple memcpy
void nmea_cpy(uint8_t *dst, uint8_t *src, uint8_t n)
{
    while (n--)
        *dst++ = *src++;
}

// ret 0 == valid crc
uint8_t nmea_crc_check()
{
    char *p;
    uint8_t crc;
    for (crc = 0, p = ubuf + 1; *p != '*'; p++)
        crc ^= *p;
    return ((((*(++p) - '0') & 0x0f) << 4) & ((*(++p) - '0') & 0x0f)) ^ crc;
}

void uart1_isr() __interrupt 4 __using 2
{
    char ch;
    char *p;
    if (RI) {
        RI = 0;                 // clear int
        if (nmea_state != NMEA_SET && nmea_state != NMEA_PARSE) {
            ch = SBUF;
            switch (ch) {
            case '$':
                uidx = 0;
                nmea_state = NMEA_START;
                break;
            case '\n':
                if (nmea_state != NMEA_START)
                    nmea_state = NMEA_NONE;
                else
                    nmea_state = NMEA_PARSE;
                break;
            default:
                if (nmea_state != NMEA_START)
                    nmea_state = NMEA_NONE;
                break;
            }
            if (nmea_state != NMEA_NONE) {
                ubuf[uidx++] = ch;
                if (nmea_state == NMEA_PARSE) {
                    // $GPRMC,232231.00,A,,,,,,,170420,,,*27
                    if (uidx >= sizeof(NMEA_TOKEN) - 1 + 12 + 1 + 5 + 12 && // 12 commas, A , *XX\r\n, hhmmss DDMMYY
                        !nmea_cmp(ubuf, NMEA_TOKEN, sizeof(NMEA_TOKEN) - 1) && // correct head
                        (p = nmea_comma(2)) && *(p + 1) == 'A' && // valid NMEA
                        nmea_comma(2) - nmea_comma(1) >= 7 && // time length
                        nmea_comma(10) - nmea_comma(9) == 7 && // date length
                        (p = nmea_comma(12)) && (*(p + 1) == '*' || *(++p + 1) == '*') &&
                        p + 6 - ubuf == uidx && // correct length
                        *(p + 4) == '\r' && *(p + 5) == '\n' && // correct tail
                        nmea_crc_check()) { // correct crc
                        nmea_state = NMEA_SET;
                        loop_gate = 1; // force main loop
                    } else {
                        // bad line, reset
                        uidx = 0;
                        nmea_state = NMEA_NONE;
                    }
                } else if (uidx >= NMEA_LINE_LEN_MAX) {
                    uidx = 0;
                    nmea_state = NMEA_NONE;
                }
            }
        }
    }
}

uint8_t char2int(char *p)
{
    return (*p - '0') * 10 + (*(p + 1) - '0');
}

__code static uint8_t doft[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
// method by Tomohiko Sakamoto, 1992, accurate for any Gregorian date
// returns 0 = Sunday, 1 = Monday, etc.
uint8_t dayofweek(uint16_t y, uint8_t m, uint8_t d)
{   // 1 <= m <= 12,  y > 1752 (in the U.K.)
    y -= m < 3;
    return (y + y / 4 - y / 100 + y / 400 + doft[m - 1] + d) % 7;
}

uint8_t isleap(uint16_t year)
{
    return (((year % 4 == 0) && (year % 100!= 0)) || (year%400 == 0));
}

__code const uint8_t monlen[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// month 1..12, year with century
uint8_t days_per_month(uint16_t year, uint8_t month)
{
    if (month == 1)
        return isleap(year) ? 29 : 28;
    else
        return monlen[month-1];
}

void nmea_apply_tz(struct tm *t)
{
    int8_t hdiff = nmea_tz_hr;
    if (t->tm_sec < 59)
        t->tm_sec ++; // just 1 sec compensation for nmea transfer delay
    if (nmea_tz_dst)
        hdiff ++;
    if (nmea_tz_min)
        if (hdiff >= 0) {
            // positive TZ
            if ((t->tm_min = t->tm_min + nmea_tz_min) >= 60) {
                t->tm_min -= 60;
                hdiff ++;
            }
        } else {
            // negative TZ
            if (nmea_tz_min > t->tm_min) {
                t->tm_min = t->tm_min + 60 - nmea_tz_min;
                hdiff --;
            } else
                t->tm_min = t->tm_min - nmea_tz_min;
        }
    if (hdiff > 0) {
        // positive TZ
        if ((t->tm_hour = t->tm_hour + hdiff) >= 24) {
            // next day
            t->tm_hour -= 24;
            if (++t->tm_mday > days_per_month(t->tm_year+1900, t->tm_mon)) {
                // next month
                t->tm_mday = 1;
                if (++t->tm_mon >= 12) {
                    // next year
                    t->tm_mon = 0; // jan
                    t->tm_year ++; 
                }
            }
        }
    } else if (hdiff < 0) {
        // negative TZ
        if (-hdiff > t->tm_hour) {
            // prev day
            t->tm_hour += 24 + hdiff;
            if (!--t->tm_mday) {
                // prev month
                if (!t->tm_mon) { // jan
                    // prev year
                    t->tm_mon = 11; // dec
                    t->tm_year --;
                } else
                    t->tm_mon --;
                t->tm_mday  =days_per_month(t->tm_year+1900, t->tm_mon);
            }
        } else
            // same day
            t->tm_hour += hdiff;
    }
}

void nmea2localtime(void)
{
    char *p;
    struct tm t;
    uint8_t hmode = DS_MASK_1224_MODE;;
    // $GPRMC,232231.00,A,,,,,,,170420,,,*27
    p = nmea_comma(9) + 1;
    t.tm_mday = char2int(p);          // day
    t.tm_mon = char2int(p+2) - 1;     // month, 0 = jan
    t.tm_year = char2int(p+4) + 100;  // year - 1900
    p = nmea_comma(1) + 1;
    t.tm_hour = char2int(p);
    t.tm_min = char2int(p+2);
    t.tm_sec = char2int(p+4);
    nmea_apply_tz(&t);

    ds_writebyte(DS_ADDR_DAY, ds_int2bcd(t.tm_mday));
    ds_writebyte(DS_ADDR_MONTH, ds_int2bcd(t.tm_mon + 1));
    ds_writebyte(DS_ADDR_YEAR, ds_int2bcd(t.tm_year - 100));
    ds_writebyte(DS_ADDR_WEEKDAY, dayofweek(t.tm_year+1900, t.tm_mon+1, t.tm_mday) + 1);
    if (!H12_12) {
        ds_writebyte(DS_ADDR_HOUR, ds_int2bcd(t.tm_hour));
    } else {
        if (t.tm_hour >= 12) {
            t.tm_hour -= 12;
            hmode |= DS_MASK_PM;
        }
        if (!t.tm_hour)
            t.tm_hour = 12;
        ds_writebyte(DS_ADDR_HOUR, ds_int2bcd(t.tm_hour) | hmode);
    }
    ds_writebyte(DS_ADDR_MINUTES, ds_int2bcd(t.tm_min));
    ds_writebyte(DS_ADDR_SECONDS, 0b10000000); // set CH, stop clock
    ds_writebyte(DS_ADDR_SECONDS, ds_int2bcd(t.tm_sec)); // clear CH, start clock
}

void nmea_save_tz(void )
{
    Delay(10);
    IapEraseSector(IAP_TZ_ADDRESS);
    Delay(10);
    IapProgramByte(IAP_TZ_HR, (uint8_t) nmea_tz_hr);
    IapProgramByte(IAP_TZ_MIN, nmea_tz_min);
    IapProgramByte(IAP_TZ_DST, nmea_tz_dst);
}

void nmea_load_tz(void )
{
    nmea_tz_hr = (int8_t) IapReadByte(IAP_TZ_HR);
    nmea_tz_min = IapReadByte(IAP_TZ_MIN);
    nmea_tz_dst = IapReadByte(IAP_TZ_DST);
    // HR after reflash will be == 0xff == -1 by default
    if (nmea_tz_hr < -12 || nmea_tz_hr > 12)
        nmea_tz_hr = 0;
    if (nmea_tz_min == 0xff ||  nmea_tz_min && nmea_tz_min != 30 && nmea_tz_min != 45)
        nmea_tz_min = 0;
    if (nmea_tz_dst == 0xff ||  nmea_tz_dst > 1)
        nmea_tz_dst = 0;
}
