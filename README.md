# STC DIY Clock Kit firmware
Firmware replacement for STC15F mcu-based DIY Clock Kit (available from banggood.com, aliexpress.com, et al.) Uses [sdcc](http://sdcc.sf.net) to build and [stcgal](https://github.com/grigorig/stcgal) to flash firmware on to STC15F204EA series microcontroller.

## features
Basic functionality is working:
* time display/set (12/24 hour modes)
* date display/set
* display auto-dim
* temperature display in C

**note this project in development and a work-in-progress**
*Pull requests are welcome.*

## TODOs
These are open items as I have run out of the limited (4k) code space. Items will likely need to be refactored, i.e. to assembly, and/or broken out as build-time selectable features using preprocessor macros rather than run-time options.

Open items:
* temperature display in C/F selectable
* alarm and chime functionality


## hardware
* DIY LED Clock kit, based on STC15F204EA and DS1302, e.g. Banggood sku 972289
* connected to PC via cheap USB-UART adapter, e.g. CP2102, CH340G.

## requirements
* linux or mac (windows untested)
* sdcc installed and in the path
* stcgal (or optionally stc-isp). Note you can either do "git clone --recursive ..." when you check this repo out, or do "git submodule update --init --recursive" in order to fetch stcgal.

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
For example, delay routines would need to be adjusted if this is different.

## disclaimers
This code is provided as-is, with NO guarantees or liabilities.
As the original firmware loaded on an STC MCU cannot be downloaded or backed up, it cannot be restored. If you are not comfortable with experimenting, I suggest obtaining another blank STC MCU and using this to test, so that you can move back to original firmware, if desired.

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

VE3LNY's adaptation of this hardware to AVR (he has some interesting AVR projects there):
http://www.qsl.net/v/ve3lny/travel_clock.html

[original firmware operation flow state diagram](docs/DIY_LED_Clock_operation_original.png)
[kit instructions w/ schematic](docs/DIY_LED_Clock.png)

