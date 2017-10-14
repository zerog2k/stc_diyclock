#include "stc15.h"
#include <stdbool.h>

#define BAUDRATE 9600

volatile char uart_char;

// buffer to hold string
#define NMEA_LINE_LEN_MAX 84

__pdata char linebuf[NMEA_LINE_LEN_MAX];
volatile uint8_t linebuf_idx = 0;

typedef enum {
    NMEA_LINE_START,
    NMEA_LINE_STOP,
    NMEA_LINE_UNKNOWN
} nmea_line_state_t;

volatile nmea_line_state_t nmea_line_state = NMEA_LINE_UNKNOWN;

void uart1_init()
{
    P_SW1 |= (1<<6); // move UART1 pins -> P3_6:rxd, P3_7:txd
    
    // UART1 use Timer2
    T2L = (65536 - (FOSC/4/BAUDRATE)) & 0xFF;
    T2H = (65536 - (FOSC/4/BAUDRATE)) >> 8;

    SM1 = 1;        // serial mode 1: 8-bit async
    AUXR |= 0x14;   // T2R: run T2, T2x12: T2 clk src sysclk/1
    AUXR |= 0x01;   // S1ST2: T2 is baudrate generator

    ES = 1;         // enable uart1 interrupt
    EA = 1;         // enable interrupts
    REN = 1;
}

void uart1_isr() __interrupt 4 __using 2
{
    if (RI)
    {
        RI = 0; //clear rx int
        uart_char = SBUF;
        if (nmea_line_state == NMEA_LINE_START) {
            // capturing a line
            if ((uart_char == '\n') || (linebuf_idx >= (NMEA_LINE_LEN_MAX-1))) {
                // reached end of line or buffer full
                REN = 0;    // stop rx
                nmea_line_state = NMEA_LINE_STOP;
            } else {
                // append buffer
                linebuf[linebuf_idx++] = uart_char;
            }
        } else {
            // not already capturing a line, wait for start
            if (uart_char == '$')
            {
                nmea_line_state = NMEA_LINE_START;     
                linebuf[linebuf_idx++] = uart_char;
            }
            // otherwise ignore char in buf
        }

        if (nmea_line_state == NMEA_LINE_STOP)
        {
            // check buffer for target string
            // TODO: consider moving out of ISR and using flags/state signalling
            // TODO: buffer entire nmea line & verify checksum
            if (memcmp(linebuf, gpstoken, GPSTOKEN_LEN) != 0)
            {
                // unwanted, so reset state, buffer, re-enable rx
                nmea_line_state = NMEA_LINE_UNKNOWN;
                linebuf_idx = 0;
                memset(linebuf, 0, NMEA_LINE_LEN_MAX);
                REN = 1;
            }
        }
        
    }
}

void putchar(char c)
{
    SBUF = c;       // put byte in buf
    while (! TI);
    TI = 0;    
}

char getchar() {
    char c;
    while (!RI)
      ;
    RI=0;
    c=SBUF;
    return c;
  }
  

bool get_nmea_ts(char *buf, uint8_t len)
{
    // calling this function will copy buffer and restart rx
    // TODO: use complete line buffer and verify
    bool success = false;
    if (nmea_line_state == NMEA_LINE_STOP)
    {
        ES = 0; // disable uart interrupt
        // received nmea line, copy to buffer
        memset(buf, 0, len);    // zero pad string buffer
        strncpy(buf, linebuf + GPSTOKEN_LEN, GPS_TIMESTAMP_LEN);
        // reset state, buffer, interrupt, and enable rx
        nmea_line_state = NMEA_LINE_UNKNOWN;
        linebuf_idx = 0;
        ES = 1;
        REN = 1;
        success = true;
    }
    return success;
}

