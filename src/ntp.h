#define BAUDRATE 57600

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

#define NTP_LEN 17

char ubuf[NTP_LEN];
int8_t uidx = 0;

// #WYYYYMMDDHHMMSS#
// 01234567890123456

void uart1_isr() __interrupt 4 __using 2
{
    char ch;
    if (RI) {
        RI = 0;                 // clear int
        ch = SBUF;
        if (ch == '#' || (ch >= '0' && ch <= '9' && uidx && ubuf[0] == '#')) {
            if (ch == '#') {
                ubuf[uidx] = ch;
                if (uidx == NTP_LEN - 1 && ubuf[0] == '#') {
                    // got entire string, set time
                    ds_writebyte(DS_ADDR_SECONDS, 0b10000000); // set CH, stop clock
                    ds_writebyte(DS_ADDR_WEEKDAY, ubuf[1]);
                    ds_writebyte(DS_ADDR_YEAR, (ubuf[4] << 4) | ubuf[5]);
                    ds_writebyte(DS_ADDR_MONTH, ((ubuf[6] & 1) << 4) | ubuf[7]);
                    ds_writebyte(DS_ADDR_DAY, (ubuf[8] << 4) | ubuf[9]);
                    ds_writebyte(DS_ADDR_HOUR, ((ubuf[10] & 3) << 4) | ubuf[11]);
                    ds_writebyte(DS_ADDR_MINUTES, ((ubuf[12] & 7) << 4) | ubuf[13]);
                    ds_writebyte(DS_ADDR_SECONDS, ((ubuf[14] & 7) << 4) | ubuf[15]);    // clear CH, start clock
                } else if (uidx) {
                    // not 1st or last # - make it 1st, skip leading garbage
                    uidx = 0;
                    ubuf[uidx] == '#';
                }
            } else
                // '0'..'9' here
                ubuf[uidx] = ch - '0';
            if (++uidx >= NTP_LEN)
                uidx = 0;
        }
    }
}
