// LED functions for 4-digit seven segment led

#include <stdint.h>

// index into ledtable[]
#define LED_a   0xa
#define LED_b   0xb
#define LED_c   0xc
#define LED_d   0xd
#define LED_e   0xe
#define LED_f   0xf
#define LED_BLANK  0x10
#define LED_DASH   0x11
#define LED_h   0x12
#define LED_dp  0x13

#ifdef WITH_LEDTABLE_RELOC
__at (0x1000)
#endif
const uint8_t ledtable[] = {
    // digit to led digit lookup table
    // dp,g,f,e,d,c,b,a
    0b11000000, // was 0b00111111, // 0
    0b11111001, //     0b00000110, // 1
    0b10100100, //     0b01011011, // 2
    0b10110000, //     0b01001111, // 3
    0b10011001, //     0b01100110, // 4
    0b10010010, //     0b01101101, // 5
    0b10000010, //     0b01111101, // 6
    0b11111000, //     0b00000111, // 7
    0b10000000, //     0b01111111, // 8
    0b10010000, //     0b01101111, // 9 with d segment
    0b10001000, //     0b01110111, // A
    0b10000011, //     0b01111100, // b
    0b11000110, //     0b00111001, // C
    0b10100001, //     0b01011110, // d
    0b10000110, //     0b01111001, // E
    0b10001110, //     0b01110001, // F
    0b11111111, //     0b00000000, // 0x10 - ' '
    0b10111111, //     0b01000000, // 0x11 - '-'
    0b10001011, //     0b01110100, // 0x12 - 'h'
    0b01111111, //     0b10000000, // 0x13 - '.'
};

// Same but with abc <-> def

#ifdef WITH_LEDTABLE_RELOC
__at (0x1100)
#endif
const uint8_t ledtable2[] = {
    0b11000000, // was 0b00111111, // 0
    0b11001111, //     0b00000110, // 1
    0b10100100, //     0b01011011, // 2
    0b10000110, //     0b01001111, // 3
    0b10001011, //     0b01100110, // 4
    0b10010010, //     0b01101101, // 5
    0b10010000, //     0b01111101, // 6
    0b11000111, //     0b00000111, // 7
    0b10000000, //     0b01111111, // 8
    0b10000010, //     0b01101111, // 9 with d segment
    0b10000001, //     0b01110111, // A
    0b10011000, //     0b01111100, // b
    0b11110000, //     0b00111001, // C
    0b10001100, //     0b01011110, // d
    0b10110000, //     0b01111001, // E
    0b10110001, //     0b01110001, // F
    0b11111111, //     0b00000000, // 0x10 - ' '
    0b10111111, //     0b01000000, // 0x11 - '-'
    0b10011001, //     0b01110100, // 0x12 - 'h'
    0b01111111, //     0b10000000, // 0x13 - '.'
};

volatile uint8_t tmpbuf[4];
__bit   dot0;
__bit   dot1;
__bit   dot2;
__bit   dot3;

volatile uint8_t dbuf[4];

#define clearTmpDisplay() { dot0=0; dot1=0; dot2=0; dot3=0; tmpbuf[0]=tmpbuf[1]=tmpbuf[2]=tmpbuf[3]=LED_BLANK; }

// N.B.: must mask off dynamic "val" here to single byte, else strange things will happen with preprocessor type promotion
// also, do not cast to uint8_t or else struct bitfields won't work correctly
#define filldisplay(pos,val,dp) { tmpbuf[pos] = val & 0xff; if (dp) dot##pos = 1;}
#define dotdisplay(pos,dp) { if (dp) dot##pos = 1;}

#define updateTmpDisplay() { uint8_t tmp; \
                        tmp=ledtable[tmpbuf[0]]; if (dot0) tmp&=0x7F; dbuf[0]=tmp; \
                        tmp=ledtable[tmpbuf[1]]; if (dot1) tmp&=0x7F; dbuf[1]=tmp; \
                        tmp=ledtable[tmpbuf[3]]; if (dot3) tmp&=0x7F; dbuf[3]=tmp; \
                        tmp=ledtable2[tmpbuf[2]]; if (dot2) tmp&=0x7F; dbuf[2]=tmp; }
                        
