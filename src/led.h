// LED functions for 4-digit seven segment display
// with the third segment rotated 180 degrees

#include <stdint.h> // uint8_t

// index into ledtable[] for some interesting characters
#define LED_BLANK  10
#define LED_DASH   11
#define LED_h      12
#define LED_dp     13
#define LED_a      14
#define LED_b      15
#define LED_c      16
#define LED_d      17
#define LED_e      18
#define LED_f      19

#ifdef WITH_ALT_LED9 
// number '9' without d segment
#define NUMBERNINE 0b10011000
#else
// number '9' with d segment on
#define NUMBERNINE 0b10010000
#endif

#ifdef WITH_LEDTABLE_RELOC
#define LEDTABLE_RELOC __at (0x1000)
#else
#define LEDTABLE_RELOC
#endif

const uint8_t LEDTABLE_RELOC ledtable[] = {
    // digit to led digit lookup table
    // dp,g,f,e,d,c,b,a
    0b11000000, //  0 - '0'  0x00
    0b11111001, //  1 - '1'
    0b10100100, //  2 - '2'
    0b10110000, //  3 - '3'
    0b10011001, //  4 - '4'
    0b10010010, //  5 - '5'
    0b10000010, //  6 - '6'
    0b11111000, //  7 - '7'
    0b10000000, //  8 - '8'  0x08
    NUMBERNINE, //  9 - '9'
    0b11111111, // 10 - ' '  0x0A
    0b10111111, // 11 - '-'
    0b10001011, // 12 - 'h'
    0b01111111, // 13 - '.'
    0b10001000, // 14 - 'A'  0x0E
    0b10000011, // 15 - 'b'
    0b11000110, // 16 - 'C'  0x10
    0b10100001, // 17 - 'd'
    0b10000110, // 18 - 'E'
    0b10001110, // 19 - 'F'
    0b11000010, // 20 - 'G'  0x14
    0b10001011, // 21 - 'h'  0x15 (same as 0x0C pos 12)
    0b11111011, // 22 - 'i'
    0b11100001, // 23 - 'J'
    0b10001010, // 24 - 'K'  0x18
    0b11000111, // 25 - 'L'
    0b11001000, // 26 - 'M'
    0b10101011, // 27 - 'N'
    0b10100011, // 28 - 'O'  0x1C
    0b10001100, // 29 - 'P'
    0b10011000, // 30 - 'Q'
    0b10101111, // 31 - 'R'
    0b10010010, // 32 - 'S'  0x20
    0b10000111, // 33 - 't'  -- was 0b11000111, //     T
    0b11100011, // 34 - 'U'
    0b11000001, // 35 - 'V'  0x23
    0b10000001, // 36 - 'W'  0x24
    0b10001001, // 37 - 'X'
    0b10010001, // 38 - 'Y'
    0b10110110, // 39 - 'Z'  0x27
}; // ledtable

const char weekDay[7][3] = {
    {'S', 'U', 'N'},
    {'M', 'O', 'N'},
    {'T', 'U', 'E'},
    {'W', 'E', 'D'},
    {'T', 'H', 'U'},
    {'F', 'R', 'I'},
    {'S', 'A', 'T'}
};


uint8_t dbuf[4]; // 7seg Display buffer

uint8_t tmpbuf[4];
__bit   dot0;
__bit   dot1;
__bit   dot2;
__bit   dot3;

#define clearTmpDisplay() { dot0=0; dot1=0; dot2=0; dot3=0; tmpbuf[0]=tmpbuf[1]=tmpbuf[2]=tmpbuf[3]=LED_BLANK; }
#define filldisplay(pos,val,dp) { tmpbuf[pos]=(uint8_t)(val); dot##pos=!!dp;}
#define dotdisplay(pos,dp) { dot##pos=!!dp;}

// rotated 7seg display means abc <-> def (bit012 <-> bit345)
#define rotated(x) ((x & 0xC0)|((x & 0x38)>>3)|((x & 0x07)<<3))

#define updateTmpDisplay() { uint8_t tmp; \
                        tmp=ledtable[tmpbuf[0]];                     if (dot0) tmp&=0x7F; dbuf[0]=tmp; \
                        tmp=ledtable[tmpbuf[1]];                     if (dot1) tmp&=0x7F; dbuf[1]=tmp; \
                        tmp=ledtable[tmpbuf[2]]; tmp = rotated(tmp); if (dot2) tmp&=0x7F; dbuf[2]=tmp; \
                        tmp=ledtable[tmpbuf[3]];                     if (dot3) tmp&=0x7F; dbuf[3]=tmp; \
                        }
                        
