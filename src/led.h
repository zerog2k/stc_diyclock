// LED functions for 4-digit seven segment display
// with the third segment rotated 180 degrees

#include <stdint.h>

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

const uint8_t
#ifndef WITHOUT_LEDTABLE_RELOC
__at (0x1000)
#endif
ledtable[]
 = {
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
#ifdef WITH_ALT_LED9
    0b10010000, //  9 - '9' with d segment
#else
    0b10011000, //  9 - '9' without d segment
#endif
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
};


#ifdef EXTRA_LEDTABLE
// ROTATED 7SEG display
// Same but with abc <-> def

const uint8_t
#ifndef WITHOUT_LEDTABLE_RELOC
__at (0x1100)
#endif
ledtable2[]
 ={
    0b11000000, // was 0b00111111, // 0
    0b11001111, //     0b00000110, // 1
    0b10100100, //     0b01011011, // 2
    0b10000110, //     0b01001111, // 3
    0b10001011, //     0b01100110, // 4
    0b10010010, //     0b01101101, // 5
    0b10010000, //     0b01111101, // 6
    0b11000111, //     0b00000111, // 7
    0b10000000, //     0b01111111, // 8
#ifdef WITH_ALT_LED9
    0b10000010, //     0b01101111, // 9 with d segment
#else
    0b10000011, //     0b01100111, // 9 without d segment
#endif
    0b11111111, //     0b00000000, // 10 - ' '
    0b10111111, //     0b01000000, // 11 - '-'
    0b10011001, //     0b01110100, // 12 - 'h'
    0b01111111, //     0b10000000, // 13 - '.'
    0b10000001, //     0b01110111, // A
    0b10011000, //     0b01111100, // b
    0b11110000, //     0b00111001, // C
    0b10001100, //     0b01011110, // d
    0b10110000, //     0b01111001, // E
    0b10110001, //     0b01110001, // F
    0b11010000, //     G
    0b10011001, //     H
    0b11011111, //     I
    0b11001100, //     J
    0b10010001, //     K
    0b11111000, //     L
    0b11000001, //     M
    0b10011101, //     N
    0b10011100, //     O
    0b10100001, //     P
    0b10000011, //     Q
    0b10111101, //     R
    0b10010010, //     S
    0b11110000, //     t  -- was 0b11000111, //     T
    0b11011100, //     U
    0b11001000, //     V
    0b10001000, //     W
    0b10001001, //     X
    0b10001010, //     Y
    0b10110110, //     Z
};
#endif // EXTRA_LEDTABLE


const char weekDay[][4] = {
      "SUN",
      "MON",
      "TUE",
      "WED",
      "THU",
      "FRI",
      "SAT",
};

uint8_t tmpbuf[4];
__bit   dot0;
__bit   dot1;
__bit   dot2;
__bit   dot3;

uint8_t dbuf[4];

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
                        
