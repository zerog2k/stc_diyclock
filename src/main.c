// STC15-based DIY LED Clock
//
// Variants: stc15f204ea, stc15w404as, stc15w408as
//
// Copyright 2016, Jens Jensen (github user 'zerog2k')
// Changes 2022, Hagen Patzke (github user 'hagen-git')
#include <stdint.h>

#include "adc.h"
#include "ds1302.h"
#include "hwconfig.h"
#include "led.h"
#include "stc15.h"

// standard defines
#define FOSC 11059200
#define WITH_DATE
#define WITH_ALARM
#define WITH_CHIME
// additional configuration
//#define SHOW_TEMP_DATE_WEEKDAY -- done in makefile
//#undef WITH_NMEA
//#undef WITH_CAPACITOR
//#undef WITH_MONTHLY_CORR
//#define DEBUG

// clear wdt
#define WDT_CLEAR() (WDT_CONTR |= 0x10)

// keyboard mode states
enum keyboard_mode {
  K_NORMAL,
  K_SET_HOUR,
  K_SET_MINUTE,
  K_SET_HOUR_12_24,
#ifdef WITH_NMEA
  K_TZ_SET_HOUR,
  K_TZ_SET_MINUTE,
  K_TZ_SET_DST,
#endif
  K_SEC_DISP,
  K_TEMP_DISP,
#ifdef WITH_DATE
  K_DATE_DISP,
  K_SET_MONTH,
  K_SET_DAY,
  K_YEAR_DISP,
#endif
  K_WEEKDAY_DISP,
#ifdef WITH_ALARM
  K_ALARM,
  K_ALARM_SET_HOUR,
  K_ALARM_SET_MINUTE,
#endif
#ifdef WITH_CHIME
  K_CHIME,
  K_CHIME_SET_SINCE,
  K_CHIME_SET_UNTIL,
#endif
#ifdef DEBUG
  K_DEBUG,
  K_DEBUG2,
  K_DEBUG3,
#endif
};

// display mode states
enum display_mode {
  M_NORMAL,
  M_SET_HOUR_12_24,
#ifdef WITH_NMEA
  M_TZ_SET_TIME,
  M_TZ_SET_DST,
#endif
  M_SEC_DISP,
  M_TEMP_DISP,
#ifdef WITH_DATE
  M_DATE_DISP,
#endif
  M_WEEKDAY_DISP,
  M_YEAR_DISP,
#ifdef WITH_ALARM
  M_ALARM,
#endif
#ifdef WITH_CHIME
  M_CHIME,
#endif
#ifdef DEBUG
  M_DEBUG,
  M_DEBUG2,
  M_DEBUG3,
#endif
};
#define NUM_DEBUG 3

#ifdef DEBUG
uint8_t hex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 18, 19};
uint8_t adc_lightval;
#endif

/* ------------------------------------------------------------------------- */
/*
void _delay_ms(uint8_t ms)
{
    // delay function, tuned for 11.092 MHz clock
    // optimized to assembler
    ms; // keep compiler from complaining?
    __asm;
        ; dpl contains ms param value
    delay$:
        mov	b, #8   ; i
    outer$:
        mov	a, #243    ; j
    inner$:
        djnz acc, inner$
        djnz b, outer$
        djnz dpl, delay$
    __endasm;
}
*/

uint8_t count;     // main loop counter
uint8_t temp;      // temperature sensor value
uint8_t lightval;  // light sensor value

// Timer 0 Interrupt Service Routine variables
volatile uint8_t displaycounter = 0; // 0.0001s = 100ns
volatile uint8_t count_100 = 0;      // 0.01s   = 10ms
volatile uint8_t count_1000 = 0;     // 0.1s    = 100ms
volatile uint8_t count_5000 = 0;     // 0.5s    = 500ms
#ifdef WITH_ALARM
volatile uint8_t count_20000;        // 2s      = 2000ms
volatile __bit blinker_slowest;
#endif
#ifdef WITH_CHIME
volatile uint16_t chime_ticks;  // 10ms inc
#endif

volatile uint16_t count_timeout;  // max 6.5536 sec
#define TIMEOUT_LONG 0xFFFF
volatile __bit blinker_slow;
volatile __bit blinker_fast;
volatile __bit loop_gate;

uint8_t dmode = M_NORMAL;  // display mode state
uint8_t kmode = K_NORMAL;
uint8_t dmode_bak = M_NORMAL;

__bit flash_01;
__bit flash_23;

uint8_t rtc_hh_bcd;
uint8_t rtc_mm_bcd;
__bit rtc_pm;
#ifdef WITH_ALARM
uint8_t alarm_hh_bcd;
uint8_t alarm_mm_bcd;
__bit alarm_pm;
__bit alarm_trigger;
__bit alarm_reset;
#endif
#ifdef WITH_CHIME
uint8_t chime_ss_bcd;  // hour since
uint8_t chime_uu_bcd;  // hour until
__bit chime_ss_pm;
__bit chime_uu_pm;
enum chime_state {
  CHIME_IDLE = 0,
  CHIME_RUNNING,
  CHIME_WAITING
};
volatile uint8_t chime_trigger = CHIME_IDLE;
#endif
__bit cfg_changed = 1;
uint8_t snooze_time;      // snooze(min)
uint8_t alarm_mm_snooze;  // next alarm time (min)
uint8_t ss;

#if defined(WITH_MONTHLY_CORR) && WITH_MONTHLY_CORR != 0
#define SEC_PER_MONTH 2592000
#if WITH_MONTHLY_CORR > 0
#define CORR_VALUE 1   // +1 sec every ~RUNTIME_PER_SEC seconds
#else                  // not WITH_MONTHLY_CORR > 0
#define CORR_VALUE -1  // -1 sec every ~RUNTIME_PER_SEC seconds
#endif                 // not WITH_MONTHLY_CORR > 0
#define RUNTIME_PER_SEC (SEC_PER_MONTH / (WITH_MONTHLY_CORR * CORR_VALUE))
volatile uint32_t corr_remaining = RUNTIME_PER_SEC;
#endif  // WITH_MONTHLY_CORR

volatile __bit S1_LONG;
volatile __bit S1_PRESSED;
volatile __bit S2_LONG;
volatile __bit S2_PRESSED;
#if NUM_SW == 3
volatile __bit S3_LONG;
volatile __bit S3_PRESSED;
#endif

volatile uint8_t debounce[NUM_SW];  // switch debounce buffer
volatile uint8_t switchcount[NUM_SW];
#define SW_CNTMAX 80  // long push

enum Event {
  EV_NONE,
  EV_S1_SHORT,
  EV_S1_LONG,
  EV_S2_SHORT,
  EV_S2_LONG,
  EV_S1S2_LONG,
#if NUM_SW == 3
  EV_S3_SHORT,
  EV_S3_LONG,
#endif
  EV_TIMEOUT,
};

#ifdef WITH_NMEA
#include "nmea.h"
#endif

volatile enum Event event;

/*
  Interrupt Service Routine, come here every 100us == 0.1ms
  - Multiplex LEDs:
    100Hz are 10ms, so we have 25 ticks per display for dimming
    But this means we have a counter to 25 and another from 0..3.
    If we go down to 78Hz, we can use a 5-bit counter from 0..31.
  - Check button status
 */
void Timer0_ISR() __interrupt 1 __using 1 {
  enum Event ev = EV_NONE;

  // Display refresh
  uint8_t pwmct = (displaycounter & 0x1F);       // 0..31
  uint8_t digit = (displaycounter >> 5) & 0x03;  // 0..3
  displaycounter++;
  if (pwmct == 0) { // next 7seg LED position
    LED_DIGITS_OFF();
    LED_SEGMENT_PORT = dbuf[digit];
    LED_DIGIT_ON(digit);
  } else if (pwmct >= lightval) { // end of LED duty cycle, turn off
    LED_DIGITS_OFF();
  }

  // Counter updates
  if (++count_100 == 100) { // 100/sec: 10 ms
    count_100 = 0;

#ifdef WITH_CHIME
    if (chime_trigger != CHIME_IDLE)
      chime_ticks++;  // increment every 10ms
#endif
    
    if (++count_1000 == 10) { // 10/sec: 100 ms
      count_1000 = 0; 
      blinker_fast = !blinker_fast;  // blink every 100ms
      loop_gate = 1;                 // every 100ms

      if (++count_5000 == 5) { // 2/sec: 500 ms
        count_5000 = 0;
        blinker_slow = !blinker_slow;  // blink every 500ms
#if defined(WITH_MONTHLY_CORR) && WITH_MONTHLY_CORR != 0
        if (!blinker_slow && corr_remaining)
          corr_remaining--;
#endif
#ifdef WITH_NMEA
        if (!blinker_slow && sync_remaining)
          if (!--sync_remaining)
            REN = 1;  // enable uart receiving
#endif
#ifdef WITH_ALARM
        if (++count_20000 == 4) { // 1/ 2sec: 2s
          count_20000 = 0;
        }
        // 500 ms on=true=1, 1500 ms off=false=0
        blinker_slowest = count_20000 == 0;
#endif
      } // count_5000
    } // count_1000

    // Check button status and debounce
#define MONITOR_S(n)                                      \
  {                                                       \
    uint8_t s = n - 1;                                    \
    /* read switch positions into sliding 8-bit window */ \
    debounce[s] = (debounce[s] << 1) | SW##n;             \
    if (debounce[s] == 0) {                               \
      /* down for at least 8 ticks */                     \
      S##n##_PRESSED = 1;                                 \
      if (!S##n##_LONG) {                                 \
        switchcount[s]++;                                 \
      }                                                   \
    } else {                                              \
      /* released or bounced */                           \
      if (S##n##_PRESSED) {                               \
        if (!S##n##_LONG) {                               \
          ev = EV_S##n##_SHORT;                           \
        }                                                 \
        S##n##_PRESSED = 0;                               \
        S##n##_LONG = 0;                                  \
        switchcount[s] = 0;                               \
      }                                                   \
    }                                                     \
    if (switchcount[s] > SW_CNTMAX) {                     \
      S##n##_LONG = 1;                                    \
      switchcount[s] = 0;                                 \
      ev = EV_S##n##_LONG;                                \
    }                                                     \
  }

    MONITOR_S(1);
    MONITOR_S(2);
#if NUM_SW == 3
    MONITOR_S(3);
#endif

    if (ev == EV_S1_LONG && S2_PRESSED) {
      S2_LONG = 1;
      switchcount[1] = 0;
      ev = EV_S1S2_LONG;
    } else if (ev == EV_S2_LONG && S1_PRESSED) {
      S1_LONG = 1;
      switchcount[0] = 0;
      ev = EV_S1S2_LONG;
    }
    if (event == EV_NONE) {
      event = ev;
    }
  } // count100

#ifdef WITH_ALARM
  if (count_timeout != 0) {
    count_timeout--;
    if (count_timeout == 0) {
      if (event == EV_NONE) {
        event = EV_TIMEOUT;
      }
    }
  }
#endif
}


// Call Timer0_ISR() 10000/sec: 0.0001 sec (100ns)
// Initialize the timer count so that it overflows after 0.0001 sec
// THTL = 0x10000 - FOSC / 12 / 10000 = 0x10000 - 92.16 = 65444 = 0xFFA4
// When 11.0592MHz clock case, set every 100us interruption
void Timer0Init(void)  // 100us @ 11.0592MHz
{
  // refer to section 7 of datasheet: STC15F2K60S2-en2.pdf
  // TMOD = 0;    // default: 16-bit auto-reload
  // AUXR = 0;    // default: traditional 8051 timer frequency of FOSC / 12
  // Initial values of TL0 and TH0 are stored in hidden reload registers: RL_TL0 and RL_TH0
  TL0 = 0xA4;  // Initial timer value
  TH0 = 0xFF;  // Initial timer value
  TF0 = 0;     // Clear overflow flag
  TR0 = 1;     // Timer0 start run
  ET0 = 1;     // Enable timer0 interrupt
  EA = 1;      // Enable global interrupt
}

// Formula was : 76-raw*64/637 - which makes use of integer mult/div routines
// Getting degF from degC using integer was not good as values were sometimes jumping by 2
// The floating point one is even worse in term of code size generated (>1024bytes...)
// Approximation for slope is 1/10 (64/637) - valid for a normal 20 degrees range
// & let's find some other trick (80 bytes - See also docs\Temp.ods file)
int8_t gettemp(uint16_t raw) {
  uint16_t val = raw;
  uint8_t temp;

  raw <<= 2;
  if (CONF_C_F) raw <<= 1;  // raw*5 (4+1) if Celsius, raw*9 (4*2+1) if Fahrenheit
  raw += val;

  if (CONF_C_F) {
    // equiv. to temp=xxxx-(9/5)*raw/10 i.e. 9*raw/50
    // see next - same for degF
    val = 6835;
    temp = 32;
  } else {
    // equiv. to temp=xxxx-raw/10 or which is same 5*raw/50
    // at 25degC, raw is 512, thus 24 is 522 and limit between 24 and 25 is 517
    // so between 0deg and 1deg, limit is 517+24*10 = 757
    // (*5 due to previous adjustment of raw value)
    val = 3785; // 5 * 757
    temp = 0;
  }
  
  while (raw < val) {
    temp++;
    val -= 50;
  }

  // add correction factor from config table
  return temp + (cfg_table[CFG_TEMP_BYTE] & CFG_TEMP_MASK) - 4;
}

// 3rd LED's dot display. Blink pattern is set when alarm is set.
void dot3display(__bit pm) {
#ifdef WITH_ALARM
  // dot 3: If alarm is on, blink for 500 ms every 2000 ms
  //        If 12h: on if pm when not blinking
  if (!CONF_12H) {  // 24h mode
    pm = CONF_ALARM_ON && blinker_slowest;// && blinker_fast;
  } else if (CONF_ALARM_ON && blinker_slowest) {
    // 12h mode case: blink 500ms, AM/PM=Off/On in another 500ms
    pm = blinker_fast;
  }
#endif
  dotdisplay(3, pm);
}

/*********************************************/
int main() {
  // SETUP
  // set photoresistor & ntc pins to open-drain output
  P1M1 |= (1 << ADC_LIGHT) | (1 << ADC_TEMP);
  P1M0 |= (1 << ADC_LIGHT) | (1 << ADC_TEMP);
  // init rtc and init/read config in RAM
  ds_init();
  ds_ram_config_init();
  ds_init_hours_to_24(); // needs access to config
#ifdef WITH_CAPACITOR
  // enable capacitor trickle charge ~1mA
  ds_writebyte(DS_ADDR_TCSDS, DS_TCS_TCON | DS_TC_D1_4KO);
#endif
  // uncomment in order to reset minutes and hours to zero.. Should not need this.
  // ds_reset_clock();
  Timer0Init();  // display refresh & switch read
#ifdef WITH_NMEA
  uart1_init();    // setup uart
  nmea_load_tz();  // read TZ/DST from eeprom
#endif

  // LOOP
  while (1) {
    enum Event ev;

    // TODO check if we could use some kind of sleep() here
    while (!loop_gate)
      /* wait for open every 100ms */;
    loop_gate = 0;  // close gate

    ev = event;
    event = EV_NONE;

    // sample ADC for the LDR (dimming), run frequently
    if (0 == (count & 3)) {
      temp = gettemp(getADCResult(ADC_TEMP));
      // auto-dimming, reduce adc range to 5 bits
#ifdef DEBUG
      adc_lightval = getADCResult8(ADC_LIGHT);
      lightval = (~(adc_lightval >> 3)) & 0x1F;
#else
      lightval = (~(getADCResult8(ADC_LIGHT) >> 3)) & 0x1F;
#endif
      // 0x00 is darkest and 0x1F is brightest
    }

    // Read RTC
    ds_readburst();
    // parse RTC
    {
      rtc_hh_bcd = rtc_table[DS_ADDR_HOUR] & DS_MASK_HOUR24;
      rtc_mm_bcd = rtc_table[DS_ADDR_MINUTES] & DS_MASK_MINUTES;
      rtc_pm = (rtc_hh_bcd > 0x11);
    }

#if defined(WITH_ALARM) || defined(WITH_CHIME)
    if (cfg_changed) {
#ifdef WITH_CHIME
      chime_ss_bcd = cfg_table[CFG_CHIME_SINCE_BYTE] >> CFG_CHIME_SINCE_SHIFT;
      chime_uu_bcd = cfg_table[CFG_CHIME_UNTIL_BYTE] & CFG_CHIME_UNTIL_MASK;
      // convert to BCD
      chime_ss_bcd = ds_int2bcd(chime_ss_bcd);
      chime_uu_bcd = ds_int2bcd(chime_uu_bcd);
#endif
#ifdef WITH_ALARM
      alarm_hh_bcd = cfg_table[CFG_ALARM_HOURS_BYTE] >> CFG_ALARM_HOURS_SHIFT;
      alarm_mm_bcd = cfg_table[CFG_ALARM_MINUTES_BYTE] & CFG_ALARM_MINUTES_MASK;
      // convert to BCD
      alarm_hh_bcd = ds_int2bcd(alarm_hh_bcd);
      alarm_mm_bcd = ds_int2bcd(alarm_mm_bcd);
      snooze_time = 0;
#endif
      cfg_changed = 0;
    }
#endif  // defined(WITH_ALARM) || defined(WITH_CHIME)

#ifdef WITH_CHIME
    // xx:00:00
    if (CONF_CHIME_ON && chime_trigger == CHIME_IDLE && !rtc_mm_bcd && !rtc_table[DS_ADDR_SECONDS]) {
      uint8_t hh = rtc_hh_bcd;
      uint8_t ss = chime_ss_bcd;
      uint8_t uu = chime_uu_bcd;
      if ((ss <= uu && hh >= ss && hh <= uu) || (ss > uu && (hh >= ss || hh <= uu)))
        chime_trigger = CHIME_RUNNING;
    }
#endif

#ifdef WITH_ALARM
    // check for alarm trigger
    // when snooze_time>0, just compare min portion
    if ((snooze_time == 0 && (alarm_hh_bcd == rtc_hh_bcd && alarm_mm_bcd == rtc_mm_bcd)) || (snooze_time > 0 && (alarm_mm_snooze == rtc_mm_bcd))) {
      if (CONF_ALARM_ON && !alarm_trigger && !alarm_reset) {
        alarm_trigger = 1;
      }
    } else {
      alarm_trigger = 0;
      alarm_reset = 0;
    }

    // S1 or S2 to stop alarm
    // Snooze by pushing S1
    if (alarm_trigger && !alarm_reset) {
      if (ev == EV_S1_SHORT) {
        // set snooze
        alarm_trigger = 0;
        snooze_time += 5;  // next alarm as 5min later
        // stop snooze after 1hour passing from the first alarm
        if (snooze_time > 60) snooze_time = 0;

        // need BCD calculation
        // alarm_mm_snooze = add_BCD(ds_int2bcd(snooze_time));
        alarm_mm_snooze = ds_int2bcd(snooze_time + ds_bcd2int(alarm_mm_bcd));

        if (alarm_mm_snooze > 0x59) alarm_mm_snooze = alarm_mm_snooze - 0x60;
        ev = EV_NONE;
      } else if (ev == EV_S2_SHORT) {
        alarm_reset = 1;
        alarm_trigger = 0;
        ev = EV_NONE;
        snooze_time = 0;
      }
    }
#endif

    /////////////////////////////////////////////////////////////////////
    // keyboard decision tree
    switch (kmode) {
      case K_SET_HOUR:
        flash_01 = 1;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          ds_hours_incr();
        else if (ev == EV_S2_SHORT)
          kmode = K_SET_MINUTE;
        break;

      case K_SET_MINUTE:
        flash_01 = 0;
        flash_23 = 1;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          ds_minutes_incr();
        else if (ev == EV_S2_SHORT)
          kmode = K_SET_HOUR_12_24;
        break;

      case K_SET_HOUR_12_24:
        dmode = M_SET_HOUR_12_24;
        if (ev == EV_S1_SHORT) {
          ds_hours_12_24_toggle();
          cfg_changed = 1;
        } else if (ev == EV_S2_SHORT) {
#ifdef WITH_NMEA
          nmea_prev_tz_hr = nmea_tz_hr;
          nmea_prev_tz_min = nmea_tz_min;
          nmea_prev_tz_dst = nmea_tz_dst;
          kmode = K_TZ_SET_HOUR;
#else
          kmode = K_NORMAL;
#endif
        }
        break;
#ifdef WITH_NMEA
      case K_TZ_SET_HOUR:
        dmode = M_TZ_SET_TIME;
        flash_01 = 1;
        flash_23 = 0;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          nmea_tz_hr = nmea_tz_hr < 12 ? nmea_tz_hr + 1 : -12;
        else if (ev == EV_S2_SHORT)
          kmode = K_TZ_SET_MINUTE;
        break;
      case K_TZ_SET_MINUTE:
        flash_01 = 0;
        flash_23 = 1;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          switch (nmea_tz_min) {
            case 0:
              nmea_tz_min = 30;
              break;
            case 30:
              nmea_tz_min = 45;
              break;
            default:
              nmea_tz_min = 0;
              break;
          }
        else if (ev == EV_S2_SHORT)
          kmode = K_TZ_SET_DST;
        break;
      case K_TZ_SET_DST:
        dmode = M_TZ_SET_DST;
        flash_01 = flash_23 = 1;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          nmea_tz_dst ^= 0x01;
        else if (ev == EV_S2_SHORT) {
          flash_01 = flash_23 = 0;
          kmode = K_NORMAL;
          if (nmea_prev_tz_hr != nmea_tz_hr ||
              nmea_prev_tz_min != nmea_tz_min ||
              nmea_prev_tz_dst != nmea_tz_dst)
            nmea_save_tz();
          sync_remaining = 10;  // allow sync adter adjusting
        }
        break;
#endif  // WITH_NMEA
      case K_TEMP_DISP:
        dmode = M_TEMP_DISP;
        if (ev == EV_S1_SHORT)
          ds_temperature_offset_incr();
        else if (ev == EV_S1_LONG)
          ds_temperature_cf_toggle();
        else if (ev == EV_S2_SHORT) {
#ifdef WITH_DATE
          kmode = K_DATE_DISP;
#else
          kmode = K_WEEKDAY_DISP;
#endif
        }
        break;

#ifdef WITH_DATE
      case K_DATE_DISP:
        dmode = M_DATE_DISP;
        if (ev == EV_S1_SHORT)
          ds_date_mmdd_toggle();
        else if (ev == EV_S1_LONG)
          kmode = CONF_SW_MMDD ? K_SET_DAY : K_SET_MONTH;
        else if (ev == EV_S2_SHORT)
          kmode = K_WEEKDAY_DISP;
        break;

      case K_SET_MONTH:
        flash_01 = 1;
        if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast))
          ds_month_incr();
        else if (ev == EV_S1_SHORT) {
          flash_01 = 0;
          kmode = CONF_SW_MMDD ? K_DATE_DISP : K_SET_DAY;
        }
        break;

      case K_SET_DAY:
        flash_23 = 1;
        if (ev == EV_S2_SHORT || (S2_LONG && blinker_fast))
          ds_day_incr();
        else if (ev == EV_S1_SHORT) {
          flash_23 = 0;
          kmode = CONF_SW_MMDD ? K_SET_MONTH : K_DATE_DISP;
        }
        break;
#endif

      case K_WEEKDAY_DISP:
        dmode = M_WEEKDAY_DISP;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          ds_weekday_incr();
        else if (ev == EV_S2_SHORT)
          // next mode is year_disp
          kmode = K_YEAR_DISP;
        break;

      case K_YEAR_DISP:
        dmode = M_YEAR_DISP;
        if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast))
          ds_year_incr();
        else if (ev == EV_S2_SHORT)
          kmode = K_NORMAL;
        break;

#ifdef DEBUG
      // To enter DEBUG mode, go to the SECONDS display, then hold S1 and S2 simultaneously.
      // S1 cycles through the DEBUG modes.
      // To exit DEBUG mode, hold S1 and S2 again.
      case K_DEBUG:
      case K_DEBUG2:
      case K_DEBUG3:
        dmode = M_DEBUG + kmode - K_DEBUG;
        if (ev == EV_S1_SHORT) {
          kmode = (kmode - K_DEBUG + 1) % NUM_DEBUG + K_DEBUG;
        } else if (ev == EV_S1S2_LONG) {
          kmode = K_SEC_DISP;
        }
        break;
#endif

      case K_SEC_DISP:
        dmode = M_SEC_DISP;
        if (ev == EV_S1_SHORT) {
#ifdef WITH_ALARM
          count_timeout = TIMEOUT_LONG;  // timeout for alarm disp
          kmode = K_ALARM;
#elif defined(WITH_CHIME)
          count_timeout = TIMEOUT_LONG;  // timeout for chime disp
          kmode = K_CHIME;
#else
          kmode = K_NORMAL;
#endif
        } else if (ev == EV_S2_SHORT) {
          ds_sec_zero();
#if defined(WITH_MONTHLY_CORR) && WITH_MONTHLY_CORR != 0
          corr_remaining = RUNTIME_PER_SEC;
#endif
        }
#ifdef DEBUG
        else if (ev == EV_S1S2_LONG)
          kmode = K_DEBUG;
#endif
        break;

#ifdef WITH_ALARM
      case K_ALARM:
        flash_01 = 0;
        flash_23 = 0;
        dmode = M_ALARM;
        if (ev == EV_TIMEOUT)
          kmode = K_NORMAL;
        else if (ev == EV_S1_SHORT) {
#ifdef WITH_CHIME
          count_timeout = TIMEOUT_LONG;  // timeout for chime disp
          kmode = K_CHIME;
#else
          kmode = K_NORMAL;
#endif
        } else if (ev == EV_S2_SHORT) {
          count_timeout = TIMEOUT_LONG;  // reset timeout on toggle
          ds_alarm_on_toggle();
          cfg_changed = 1;
        } else if (ev == EV_S2_LONG) {
          count_timeout = 0;  // infinite adjusting
          kmode = K_ALARM_SET_HOUR;
        }
        break;

      case K_ALARM_SET_HOUR:
        flash_01 = 1;
        alarm_reset = 1;  // don't trigger while setting
        if (ev == EV_S2_SHORT)
          kmode = K_ALARM_SET_MINUTE;
        else if (ev == EV_S1_SHORT || S1_LONG && blinker_fast) {
          ds_alarm_hours_incr();
          cfg_changed = 1;
        }
        break;

      case K_ALARM_SET_MINUTE:
        flash_01 = 0;
        flash_23 = 1;
        alarm_reset = 1;
        if (ev == EV_S2_SHORT) {
          count_timeout = TIMEOUT_LONG;  // timeout for alarm disp
          kmode = K_ALARM;
        } else if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast)) {
          ds_alarm_minutes_incr();
          cfg_changed = 1;
        }
        break;
#endif  // WITH_ALARM

#ifdef WITH_CHIME
      case K_CHIME:
        flash_01 = 0;
        flash_23 = 0;
        dmode = M_CHIME;
        if (ev == EV_S1_SHORT || ev == EV_TIMEOUT) {
          kmode = K_NORMAL;
        } else if (ev == EV_S2_SHORT) {
          count_timeout = TIMEOUT_LONG;  // reset timeout on toggle
          ds_chime_on_toggle();
          cfg_changed = 1;
        } else if (ev == EV_S2_LONG) {
          count_timeout = 0;  // infinite adjusting
          kmode = K_CHIME_SET_SINCE;
        }
        break;

      case K_CHIME_SET_SINCE:
        flash_01 = 1;
        if (ev == EV_S2_SHORT)
          kmode = K_CHIME_SET_UNTIL;
        else if (ev == EV_S1_SHORT || S1_LONG && blinker_fast) {
          ds_chime_start_incr();
          cfg_changed = 1;
        }
        break;

      case K_CHIME_SET_UNTIL:
        flash_01 = 0;
        flash_23 = 1;
        if (ev == EV_S2_SHORT) {
          count_timeout = TIMEOUT_LONG;  // timeout for chime disp
          kmode = K_CHIME;
        } else if (ev == EV_S1_SHORT || (S1_LONG && blinker_fast)) {
          ds_chime_until_incr();
          cfg_changed = 1;
        }
        break;
#endif  // WITH_CHIME

      case K_NORMAL:
      default:
        flash_01 = 0;
        flash_23 = 0;

        dmode = M_NORMAL;
        if (count_timeout)
          count_timeout = 0;  // no timeout for normal (time display) mode

        if (ev == EV_S1_SHORT)
          kmode = K_SEC_DISP;
        else if (ev == EV_S2_LONG)
          kmode = K_SET_HOUR;
        else if (ev == EV_S2_SHORT)
          kmode = K_TEMP_DISP;
#if NUM_SW == 3
        else if (ev == EV_S3_LONG) {
          LED = !LED;
        }
#endif
    };

    /////////////////////////////////////////////////////////////////////
    // display execution tree

    clearTmpDisplay();

    dmode_bak = dmode;

#ifdef SHOW_TEMP_DATE_WEEKDAY
    if (dmode == M_NORMAL && kmode == K_NORMAL) {
      ss = rtc_table[DS_ADDR_SECONDS];
      if (ss < 0x20)
        dmode = M_NORMAL;
      else if (ss < 0x25)
        dmode = M_TEMP_DISP;
      else if (ss < 0x30)
        dmode = M_DATE_DISP;
      else if (ss < 0x35)
        dmode = M_WEEKDAY_DISP;
    }
#endif

#if defined(WITH_MONTHLY_CORR) && WITH_MONTHLY_CORR != 0
    if (!corr_remaining && rtc_table[DS_ADDR_SECONDS] == 0x51) {
      ds_writebyte(DS_ADDR_SECONDS, rtc_table[DS_ADDR_SECONDS] + CORR_VALUE);
      corr_remaining = RUNTIME_PER_SEC;
    }
#endif

    switch (dmode) {
      case M_NORMAL:
#ifdef WITH_ALARM
      case M_ALARM:
#endif
#ifdef WITH_CHIME
      case M_CHIME:
#endif
      {
        uint8_t hh = rtc_hh_bcd;
        uint8_t mm = rtc_mm_bcd;
        __bit pm = 0;

#ifdef WITH_ALARM
        if (dmode == M_ALARM) {
          hh = alarm_hh_bcd;
          mm = alarm_mm_bcd;
        }
#endif

#ifdef WITH_CHIME
        if (dmode == M_CHIME) {
          hh = chime_ss_bcd;
          mm = chime_uu_bcd;
        }
#endif

        if (!flash_01 || blinker_fast || S1_LONG) {
          if (CONF_12H) { // here we adjust the DISPLAY for 12h (if needed)
            if (hh > 0x11) {
              pm = 1;    // PM
              hh = ds_int2bcd(ds_bcd2int(hh) - 12);
            }
            if (hh == 0x00) {
              hh = 0x12;
            }
          }
          uint8_t h0 = hh >> 4;
          if (CONF_12H && h0 == 0) {
            h0 = LED_BLANK;
          }
          filldisplay(0, h0, 0);
          filldisplay(1, hh & 0x0F, 0);
        }

        if (!flash_23 || blinker_fast || S1_LONG) {
#ifdef WITH_CHIME
          if (dmode == M_CHIME) {
            // remove leading zero in chime stop hr
            uint8_t m0 = mm >> 4;
            if (CONF_12H && m0 == 0) {
              m0 = LED_BLANK;
            }
            filldisplay(2, m0, 0);
          } else
#endif
            filldisplay(2, mm >> 4, 0);
            filldisplay(3, mm & 0x0F, 0);
        }
        if (blinker_slow || dmode != M_NORMAL) {
#ifdef WITH_CHIME
          if (dmode != M_CHIME) {
#endif
            dotdisplay(1, 1);
            dotdisplay(2, 1);
#ifdef WITH_CHIME
          }
#endif
        }
#ifdef WITH_CHIME
        if (dmode == M_CHIME) {
          dotdisplay(2, CONF_CHIME_ON);
          dotdisplay(1, chime_ss_pm);
        }
#endif
        dot3display(pm);
        break;
      }
      case M_SET_HOUR_12_24:
        if (CONF_12H) {
          filldisplay(1, 1, 0);
          filldisplay(2, 2, 0);
        } else {
          filldisplay(1, 2, 0);
          filldisplay(2, 4, 0);
        }
        filldisplay(3, LED_h, 0);
        break;

#ifdef WITH_NMEA
      case M_TZ_SET_TIME: {
        int8_t hh = nmea_tz_hr;
        if (hh < 0) {
          hh = -hh;
          dotdisplay(3, 1);
        } else
          dotdisplay(3, 0);
        dotdisplay(2, 1);
        if (!flash_01 || blinker_fast || S1_LONG) {
          if (hh >= 10) {
            filldisplay(0, 1, 0);
          } else {
            filldisplay(0, LED_BLANK, 0);
          }
          filldisplay(1, hh % 10, 0);
        }
        if (!flash_23 || blinker_fast || S1_LONG) {
          filldisplay(2, nmea_tz_min / 10, 0);
          filldisplay(3, nmea_tz_min % 10, 0);
        }
        break;
      }
      case M_TZ_SET_DST:
        filldisplay(0, 'D' - 'A' + LED_a, 0);
        filldisplay(1, 'S' - 'A' + LED_a, 0);
        filldisplay(2, 'T' - 'A' + LED_a, 0);
        filldisplay(3, nmea_tz_dst, 0);
        break;
#endif

      case M_SEC_DISP: {
        dotdisplay(0, 0);
        dotdisplay(1, blinker_slow);
        filldisplay(2, ((rtc_table[DS_ADDR_SECONDS] & DS_MASK_SECONDS_TENS) >> 4), blinker_slow);
        filldisplay(3, (rtc_table[DS_ADDR_SECONDS] & DS_MASK_SECONDS_UNITS), 0);
        dot3display(0);
      } break;

#ifdef WITH_DATE
      case M_DATE_DISP:
        if (!flash_01 || blinker_fast || S2_LONG) {
          if (!CONF_SW_MMDD) {
            filldisplay(0, rtc_table[DS_ADDR_MONTH] >> 4, 0);  // tenmonth ( &MASK_TENS useless, as MSB bits are read as '0')
            filldisplay(1, rtc_table[DS_ADDR_MONTH] & DS_MASK_MONTH_UNITS, 0);
          } else {
            filldisplay(2, rtc_table[DS_ADDR_MONTH] >> 4, 0);  // tenmonth ( &MASK_TENS useless, as MSB bits are read as '0')
            filldisplay(3, rtc_table[DS_ADDR_MONTH] & DS_MASK_MONTH_UNITS, 0);
          }
        }

        if (!flash_23 || blinker_fast || S2_LONG) {
          if (!CONF_SW_MMDD) {
            filldisplay(2, rtc_table[DS_ADDR_DAY] >> 4, 0);                 // tenday   ( &MASK_TENS useless)
            filldisplay(3, rtc_table[DS_ADDR_DAY] & DS_MASK_DAY_UNITS, 0);  // day
          } else {
            filldisplay(0, rtc_table[DS_ADDR_DAY] >> 4, 0);                 // tenday   ( &MASK_TENS useless)
            filldisplay(1, rtc_table[DS_ADDR_DAY] & DS_MASK_DAY_UNITS, 0);  // day
          }
        }
        dotdisplay(1, 1);
        dot3display(0);
        break;
#endif

      case M_WEEKDAY_DISP: {
        uint8_t wd;

        wd = rtc_table[DS_ADDR_WEEKDAY] - 1;

        filldisplay(1, weekDay[wd][0] - 'A' + LED_a, 0);
        filldisplay(2, weekDay[wd][1] - 'A' + LED_a, 0);
        filldisplay(3, weekDay[wd][2] - 'A' + LED_a, 0);

        dot3display(0);
      } break;

      case M_YEAR_DISP:
        // fix upper 2 digit as 20
        filldisplay(0, 2, 0);
        filldisplay(1, 0, 0);

        filldisplay(2, (rtc_table[DS_ADDR_YEAR] >> 4) & (DS_MASK_YEAR_TENS >> 4), 0);
        filldisplay(3, rtc_table[DS_ADDR_YEAR] & DS_MASK_YEAR_UNITS, 0);
        break;

      case M_TEMP_DISP: {
        uint8_t td = ds_int2bcd(temp);
        filldisplay(0, (td >> 4), 0);
        filldisplay(1, (td & 0x0F), 0);
        filldisplay(2, CONF_C_F ? LED_f : LED_c, 1);
        // if (temp<0) filldisplay( 3, LED_DASH, 0);  -- temp defined as uint16, cannot be <0
        dot3display(0);
      } break;

#ifdef DEBUG
      case M_DEBUG: {
        // seconds, loop counter, blinkers, S1/S2, keypress events
        uint8_t cc = count;
        if (S1_PRESSED || S2_PRESSED) {
          filldisplay(0, !!(S1_PRESSED || S2_PRESSED) ? LED_DASH : LED_BLANK, ev == EV_S1_SHORT || ev == EV_S2_SHORT);
          filldisplay(1, !!(S1_LONG || S2_LONG) ? LED_DASH : LED_BLANK, ev == EV_S1_LONG || ev == EV_S2_LONG);
        } else {
          filldisplay(0, (rtc_table[DS_ADDR_SECONDS] >> 4) & (DS_MASK_SECONDS_TENS >> 4), 0);
          filldisplay(1, rtc_table[DS_ADDR_SECONDS] & DS_MASK_SECONDS_UNITS, blinker_slow);
        }
        filldisplay(2, hex[cc >> 4 & 0x0F], ev != EV_NONE);
        filldisplay(3, hex[cc & 0x0F], blinker_slow & blinker_fast);
        break;
      }
      case M_DEBUG2: {
        // photoresistor adc and lightval
        filldisplay(0, hex[adc_lightval >> 4], 0);
        filldisplay(1, hex[adc_lightval & 0x0F], 0);
        filldisplay(2, hex[lightval >> 4], 0);
        filldisplay(3, hex[lightval & 0x0F], 0);
        break;
      }
      case M_DEBUG3: {
        // thermistor adc
        uint16_t rt = getADCResult(ADC_TEMP);
        filldisplay(0, hex[rt >> 12], 0);
        filldisplay(1, hex[rt >> 8 & 0x0F], 0);
        filldisplay(2, hex[rt >> 4 & 0x0F], 0);
        filldisplay(3, hex[rt & 0x0F], 0);
        break;
      }
#endif
    }

    dmode = dmode_bak;  // back to original dmode

#ifdef WITH_ALARM
    if (alarm_trigger && !alarm_reset) {
      if (blinker_slow && blinker_fast) {
        clearTmpDisplay();
        BUZZER_ON;
      } else {
        BUZZER_OFF;
      }
    } else {
      BUZZER_OFF;
    }
#endif

#ifdef WITH_CHIME
    switch (chime_trigger) {
      case CHIME_RUNNING:  // ~100ms chime
        BUZZER_ON;
        for (chime_ticks = 0; chime_ticks < 10;)
          ;
        BUZZER_OFF;
        chime_trigger = CHIME_WAITING;
      case CHIME_WAITING:  // wait > 1sec until rtc sec changed
        if (chime_ticks > 150)
          chime_trigger = CHIME_IDLE;  // stop chime
        break;
      default:
        break;
    }
#endif

    __critical {
      updateTmpDisplay();
    }

    count++;
    WDT_CLEAR();

#ifdef WITH_NMEA
    if (nmea_state == NMEA_SET) {
      if (!sync_remaining) {
        // time passed - allowed to sync
        nmea2localtime();
        REN = 0;  // disable uart receiving
        sync_remaining = MIN_NMEA_PAUSE;
      }
      uidx = 0;
      nmea_state = NMEA_NONE;
    }
#endif
  } // while (1)

} // main

//eof
