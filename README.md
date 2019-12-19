# STC DIY Clock Kit firmware

Firmware replacement for STC15F mcu-based DIY Clock Kit (available from banggood [see below for link], aliexpress, et al.) Uses [sdcc](http://sdcc.sf.net) to build and [stcgal](https://github.com/grigorig/stcgal) to flash firmware on to STC15F204EA (and STC15W408AS) series microcontroller.

![Image of Banggood SKU972289](http://img.banggood.com/thumb/large/2014/xiemeijuan/03/SKU203096/A3.jpg?p=WX0407753399201409DA)

[link to Banggood product page for SKU 972289](http://www.banggood.com/DIY-4-Digit-LED-Electronic-Clock-Kit-Temperature-Light-Control-Version-p-972289.html?p=WX0407753399201409DA)

## features
* time display/set (12/24 hour modes)
* date display/set (with reversible MM/YY, YY/MM display)
* day of week
* year
* seconds display/reset
* display auto-dim
* temperature display in C or F (with user-defined offset adjustment)
* alarm with snooze
* hourly chime

## Experimental support
* time sync to GPS receiver outputting serial NMEA data
  * on `gps` branch: https://github.com/zerog2k/stc_diyclock/tree/gps
  * for STC15W408AS or STC15W404AS (sorry no STC15F204EA, not enough ram/code, no hw uart)
  * very experimental at this point (help wanted to polish this)

**note this project in development and a work-in-progress**
*Pull requests are welcome.*

## TODOs
* time sync to WWVB radio receiver module (for STC15W408AS)

## hardware

* DIY LED Clock kit, based on STC15F204EA and DS1302, e.g. [Banggood SKU 972289](http://www.banggood.com/DIY-4-Digit-LED-Electronic-Clock-Kit-Temperature-Light-Control-Version-p-972289.html?p=WX0407753399201409DA)
* connected to PC via cheap USB-UART adapter, e.g. CP2102, CH340G. [Banggood: CP2102 USB-UART adapter](http://www.banggood.com/CJMCU-CP2102-USB-To-TTLSerial-Module-UART-STC-Downloader-p-970993.html?p=WX0407753399201409DA)

## connection
| P1 header | UART adapter |
|-----------|--------------|
| P3.1      | RXD          |
| P3.0      | TXD          |
| GND       | GND          |
| 5V        | 5V           |

## requirements
* linux or mac (windows untested, but should work)
* sdcc installed and in the path (recommend sdcc >= 3.5.0)
* stcgal (or optionally stc-isp). Note you can either do "git clone --recursive ..." when you check this repo out, or do "git submodule update --init --recursive" in order to fetch stcgal.

## usage
choose platformio or traditional make build

### platformio support
_experimental_

* assumes you have platformio installed
* choose which mcu you are building for by uncommenting one `env_default` in `platformio.ini`
* adjust `upload_port` as needed in `platformio.ini`

### traditional make
```
make clean
make
make flash
```

#### make options
* override default serial port:
`STCGALPORT=/dev/ttyUSB0 make flash`

* add other options:
`STCGALOPTS="-l 9600 -b 9600" make flash`

* flashing STC15W408AS:
`STCGALPROT="stc15" make flash`

## pre-compiled binaries
If you like, you can try pre-compiled binaries here:
https://github.com/zerog2k/stc_diyclock/releases

## use STC-ISP flash tool
Instead of stcgal, you could alternatively use the official stc-isp tool, e.g stc-isp-15xx-v6.85I.exe, to flash.
A windows app, but also works fine for me under mac and linux with wine.

~~
**note** due to optimizations that make use of "eeprom" section for holding lookup tables, if you are using 4k flash model mcu AND if using stc-isp tool, you must flash main.hex (as code file) and eeprom.hex (as eeprom file). (Ignore stc-isp warning about exceeding space when loading code file.)
To generate eeprom.hex, run:
```
make eeprom
```
~~

## clock assumptions
For STC15F204EA, some of the code assumes 11.0592 MHz internal RC system clock (set by stc-isp or stcgal).
For example, delay routines might need to be adjusted if this is different. (Most timing has been moved to hardware timers.)

## disclaimers
This code is provided as-is, with NO guarantees or liabilities.
As the original firmware loaded on an STC MCU cannot be downloaded or backed up, it cannot be restored. If you are not comfortable with experimenting, I suggest obtaining another blank STC MCU and using this to test, so that you can move back to original firmware, if desired.

### references
http://www.stcmcu.com (mostly in Chinese)

stc15f204ea english datasheet:
http://www.stcmcu.com/datasheet/stc/stc-ad-pdf/stc15f204ea-series-english.pdf

stc15w408as english datasheet:
http://www.stcmicro.com/datasheet/STC15F2K60S2-en2.pdf

sdcc user guide:
http://sdcc.sourceforge.net/doc/sdccman.pdf

some examples with NRF24L01+ board:
http://jjmz.free.fr/?tag=stc15l204

Maxim DS1302 datasheet:
http://datasheets.maximintegrated.com/en/ds/DS1302.pdf

VE3LNY's adaptation of this hardware to AVR (he has some interesting AVR projects there):
http://www.qsl.net/v/ve3lny/travel_clock.html

## diagrams
### new operation
[new firmware operation flow diagram](docs/DIY_LED_Clock_operation_new.png)

### original operation
[original firmware operation flow state diagram](docs/DIY_LED_Clock_operation_original.png)

### schematics
Kit instructions w/ schematic: [scan](docs/DIY_LED_Clock.png) | [PDF](http://img.banggood.com/file/products/20170116024635SKU203096.pdf)


### chat
[![Join the chat at https://gitter.im/zerog2k/stc_diyclock](https://badges.gitter.im/zerog2k/stc_diyclock.svg)](https://gitter.im/zerog2k/stc_diyclock?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

