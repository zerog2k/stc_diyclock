# STC DIY Clock Kit firmware
Firmware reconstruction attempt for STC15F mcu-based DIY Clock Kit (available from banggood.com, aliexpress.com, et al.) Uses [sdcc](http://sdcc.sf.net) to build and [stcgal](https://github.com/grigorig/stcgal) to flash firmware on to STC15F204EA series microcontroller.

## TODOs
**note this is currently work-in-progress, unfinished state**

* led display. Mostly done, needs blink/colon/flash/decimal points.
* ds1302 interaction. Mostly done, reads rtc. Still needs time setting routines.
* temperature measurement
* ui: button scan / settings, some kind of menu
* alarm/chime
* light sensor, display auto-dimming. Done.

## hardware
* DIY LED Clock kit, based on STC15F204EA and DS1302, e.g. Banggood sku 972289
* uC connected to PC via cheap USB-UART adapter, e.g. CP2102.

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
`STCGALPORT=/dev/ttyUSB0 make flash`

* add other options:
`STCGALOPTS="-l 9600 -b 9600" make flash`

## use STC-ISP flash tool
Instead of stcgal, you could alternatively use the official stc-isp tool, e.g stc-isp-15xx-v6.85I.exe, to flash.
A windows app, but also works fine for me under mac and linux with wine.

## clock assumptions
Some of the code assumes 11.0592 MHz internal RC system clock (set by stc-isp or stcgal).
Specifically, the soft serial code (src/soft_serial/serial.c, SYSCLK) and delay routines would need to be adjusted if this is different.

### references
http://www.stcmcu.com (mostly in Chinese)

stc15f204ea english datasheet:
http://www.stcmcu.com/datasheet/stc/stc-ad-pdf/stc15f204ea-series-english.pdf

sdcc user guide:
http://sdcc.sourceforge.net/doc/sdccman.pdf

some examples with NRF24L01+ board:
http://jjmz.free.fr/?tag=stc15l204

Maxim DS1302 datasheet:
http://datasheets.maximintegrated.com/en/ds/DS1302.pdf

[kit instructions w/ schematic](docs/DIY_LED_Clock.png)

