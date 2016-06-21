SDCCOPTS ?= --iram-size 256 --code-size 4089 --xram-size 0 --data-loc 0x30 --disable-warning 126 --disable-warning 59
STCGAL ?= stcgal/stcgal.py
STCGALOPTS ?= 
STCGALPORT ?= /dev/ttyUSB0
FLASHFILE ?= main.hex
SYSCLK ?= 11059

SRC = src/adc.c src/ds1302.c

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: main

build/%.rel: src/%.c
	mkdir -p $(dir $@)
	sdcc $(SDCCOPTS) -o $@ -c $<

main: $(OBJ)
	sdcc -o build/ src/$@.c $(SDCCOPTS) $^
	cp build/$@.ihx $@.hex
	
flash:
	$(STCGAL) -p $(STCGALPORT) -P stc15a -t $(SYSCLK) $(STCGALOPTS) $(FLASHFILE) src/eeprom.hex

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*

