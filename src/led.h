// LED functions for 4-digit seven segment led

#include <stdint.h>

// index into ledtable[]
// To define all alphabets sequentially, move LED_a - LED_f after LED_dp
#define LED_BLANK  10
#define LED_DASH   11
#define LED_h   12
#define LED_dp  13
#define LED_a   14 //0xa
#define LED_b   15 //0xb
#define LED_c   16 //0xc
#define LED_d   17 //0xd
#define LED_e   18 //0xe
#define LED_f   19 //0xf

const uint8_t
#ifndef WITHOUT_LEDTABLE_RELOC
__at (0x1000)
#endif
ledtable[]
 = {
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
#ifdef WITH_ALT_LED9
    0b10010000, //     0b01101111, // 9 with d segment
#else
    0b10011000, //     0b01100111, // 9 without d segment
#endif
    0b11111111, //     0b00000000, // 10 - ' '
    0b10111111, //     0b01000000, // 11 - '-'
    0b10001011, //     0b01110100, // 12 - 'h'
    0b01111111, //     0b10000000, // 13 - '.'
    0b10001000, //     0b01110111, // A
    0b10000011, //     0b01111100, // b
    0b11000110, //     0b00111001, // C
    0b10100001, //     0b01011110, // d
    0b10000110, //     0b01111001, // E
    0b10001110, //     0b01110001, // F
    0b11000010, //     G  0x14
    0b10001011, //     H  
    0b11111011, //     I
    0b11100001, //     J
    0b10001010, //     K  0x18
    0b11000111, //     L
    0b11001000, //     M
    0b10101011, //     N
    0b10100011, //     O  0x1c
    0b10001100, //     P
    0b10011000, //     Q
    0b10101111, //     R
    0b10010010, //     S  0x20
    0b11111000, //     T
    0b11100011, //     U
    0b11000001, //     V  0x23
    0b10000001, //     W  0x24
    0b10001001, //     X
    0b10010001, //     Y
    0b10110110, //     Z  0x27
};

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
    0b11000111, //     T
    0b11011100, //     U
    0b11001000, //     V
    0b10001000, //     W
    0b10001001, //     X
    0b10001010, //     Y
    0b10110110, //     Z
};

const char weekDay[][4] = {
      "SUN",
      "MON",
      "TUE",
      "WED",
      "THU",
      "FRI",
      "SAT",
};

// each four characters for the 7-segments display
uint8_t framebuffer[4];
__bit   dot0;
__bit   dot1;
__bit   dot2;
__bit   dot3;

uint8_t dbuf[4];

#define clearTmpDisplay() { dot0=0; dot1=0; dot2=0; dot3=0; framebuffer[0]=framebuffer[1]=framebuffer[2]=framebuffer[3]=LED_BLANK; }

#define filldisplay(pos,val,dp) { framebuffer[pos]=(uint8_t)(val); if (dp) dot##pos=1;}
#define dotdisplay(pos,dp) { if (dp) dot##pos=1;}

#define syncFramebuffer() { uint8_t tmp; \
                        tmp=ledtable[framebuffer[0]]; if (dot0) tmp&=0x7F; dbuf[0]=tmp; \
                        tmp=ledtable[framebuffer[1]]; if (dot1) tmp&=0x7F; dbuf[1]=tmp; \
                        tmp=ledtable[framebuffer[3]]; if (dot3) tmp&=0x7F; dbuf[3]=tmp; \
                        tmp=ledtable2[framebuffer[2]]; if (dot2) tmp&=0x7F; dbuf[2]=tmp; }
                        
