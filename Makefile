SDCC ?= sdcc
STCCODESIZE ?= 4089
SDCCOPTS ?= --code-size $(STCCODESIZE) --xram-size 0 --data-loc 0x30 --disable-warning 126 --disable-warning 59
STCGAL ?= stcgal/stcgal.py
STCGALOPTS ?= 
STCGALPORT ?= /dev/ttyUSB0
STCGALPROT ?= auto
FLASHFILE ?= main.hex
SYSCLK ?= 11059
CFLAGS ?= -DWITH_ALT_LED9 -DWITHOUT_LEDTABLE_RELOC -DSHOW_TEMP_DATE_WEEKDAY

#
# You need to select which hardware you want to compile for:
#

# the most common one:
HARDWARE=default

# STC15W408AS + voice + 3 buttons:
#HARDWARE=stc15w408as

# one (rare) variant
#HARDWARE=modelc



SDCCOPTS+=-DHARDWARE=\"hal/$(HARDWARE).h\"

SRC = src/adc.c src/ds1302.c

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: main

build/%.rel: src/%.c src/%.h
	mkdir -p $(dir $@)
	$(SDCC) $(SDCCOPTS) -o $@ -c $<

main: $(OBJ)
	$(SDCC) -o build/ src/$@.c $(SDCCOPTS) $(CFLAGS) $^
	@ tail -n 5 build/main.mem | head -n 2
	@ tail -n 1 build/main.mem
	cp build/$@.ihx $@.hex

eeprom:
	sed -ne '/:..1/ { s/1/0/2; p }' main.hex > eeprom.hex

flash:
	$(STCGAL) -p $(STCGALPORT) -P $(STCGALPROT) -t $(SYSCLK) $(STCGALOPTS) $(FLASHFILE)

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*

cpp: SDCCOPTS+=-E
cpp: main
