SDCCOPTS ?= --iram-size 256
COMPORT ?= /dev/ttyUSB1
STCGAL ?= stcgal.py
FLASHFILE ?= blinky.bin

SRC = src/soft_serial/serial.c

BINDIR ?= bin

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: blinky makebin

build/%.rel: src/%.c
	mkdir -p $(dir $@)
	sdcc $(SDCCOPTS) -o $@ -c $<

blinky: $(OBJ)
	sdcc -o build/ src/$@.c $(SDCCOPTS) $^
	cp build/$@.ihx $@.hex
	
makebin:
	makebin -p build/blinky.ihx blinky.bin

flash:
	$(STCGAL) -p $(COMPORT) -P stc15 $(FLASHFILE)

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*

