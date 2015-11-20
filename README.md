# STC15F204EA blinky example
Simple blinky example which uses [sdcc](http://sdcc.sf.net) to build and [stcgal](https://github.com/grigorig/stcgal) to flash firmware on to STC15F204EA series microcontroller.

## hardware
Hardware is assumed to be a generic LED with anode (+) connected to VCC (3.3-5V) through current limiting resistor (~1kohm) with cathode (-) connected to LED pin (as defined in src/blinky.c) of uC.
uC connected to PC via cheap USB-UART adapter, e.g. CP2102.

## requirements
* linux or mac (windows untested)
* sdcc installed and in the path
* stcgal (or optionally stc-isp)

## usage
```
make clean
make
make flash
```

## options
* override default serial port:
`PORT=/dev/ttyUSB0 make flash`

## use STC-ISP flash tool
Instead of stcgal, you can use official stc-isp tool, e.g stc-isp-15xx-v6.85I.exe, to flash.
A windows app, but worked fine for me under mac and linux with wine.
It can take the .hex or .bin.

## clock assumptions
Some of the code assumes 11.0592 MHz internal RC system clock (set by stc-isp or stcgal).
Specifically, the soft serial code (src/soft_serial/serial.c, SYSCLK) and delay routines (src/blinky.c) would need to be adjusted if this is different.

### references
http://www.stcmcu.com (mostly in Chinese)

stc15f204ea english datasheet:
http://www.stcmcu.com/datasheet/stc/stc-ad-pdf/stc15f204ea-series-english.pdf

sdcc user guide:
http://sdcc.sourceforge.net/doc/sdccman.pdf

some examples with NRF24L01+ board:
http://jjmz.free.fr/?tag=stc15l204

