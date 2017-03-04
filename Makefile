SDCC ?= sdcc
STCCODESIZE ?= 4089
SDCCOPTS ?= --iram-size 256 --code-size $(STCCODESIZE) --xram-size 0 --data-loc 0x30 --disable-warning 126 --disable-warning 59
SDCCREV ?= -Dstc15f204ea
STCGAL ?= stcgal/stcgal.py
STCGALOPTS ?= 
STCGALPORT ?= /dev/ttyUSB0
STCGALPROT ?= stc15a
FLASHFILE ?= main.hex
SYSCLK ?= 11059

SRC = src/adc.c src/ds1302.c

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: main

build/%.rel: src/%.c src/%.h
	mkdir -p $(dir $@)
	$(SDCC) $(SDCCOPTS) $(SDCCREV) -o $@ -c $<

main: $(OBJ)
	$(SDCC) -o build/ src/$@.c $(SDCCOPTS) $(SDCCREV) $^
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

