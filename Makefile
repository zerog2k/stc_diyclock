SDCCOPTS ?= --iram-size 256
PORT ?= /dev/ttyUSB0
STCGAL ?= stcgal/stcgal.py
FLASHFILE ?= main.hex
SYSCLK ?= 11059

SRC = src/soft_serial/serial.c

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: main

build/%.rel: src/%.c
	mkdir -p $(dir $@)
	sdcc $(SDCCOPTS) -o $@ -c $<

blinky: $(OBJ)
	sdcc -o build/ src/$@.c $(SDCCOPTS) $^
	cp build/$@.ihx $@.hex
	
flash:
	$(STCGAL) -p $(PORT) -P stc15a -t $(SYSCLK) $(FLASHFILE)

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*

