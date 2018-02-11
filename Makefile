SDCC ?= sdcc
STCCODESIZE ?= 8184
SDCCOPTS ?= --iram-size 256 --code-size $(STCCODESIZE) --xram-size 256  --data-loc 0x30 --disable-warning 126 --disable-warning 59
SDCCREV ?= -Dstc15f204ea
STCGAL ?= stcgal/stcgal.py
STCGALOPTS ?= 
STCGALPORT ?= /dev/ttyUSB0
STCGALPROT ?= stc15
FLASHFILE ?= main.hex
SYSCLK ?= 11059
CFLAGS ?= -DWITH_LEDTABLE_RELOC

SRC = src/adc.c src/ds1302.c

OBJ=$(patsubst src%.c,build%.rel, $(SRC))

all: main

build/%.rel: src/%.c src/%.h
	mkdir -p $(dir $@)
	$(SDCC) $(SDCCOPTS) $(SDCCREV) -o $@ -c $<

main: $(OBJ)
	$(SDCC) -o build/ src/$@.c $(SDCCOPTS) $(SDCCREV) $(CFLAGS) $^
	@ tail -n 5 build/main.mem 
	cp build/$@.ihx $@.hex
	
eeprom:
	sed -ne '/:..2/ { s/2/0/2; p }' main.hex > eeprom.hex

flash:
	$(STCGAL) -p $(STCGALPORT) -P $(STCGALPROT) -t $(SYSCLK) $(STCGALOPTS) $(FLASHFILE)

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*

cpp: SDCCOPTS+=-E
cpp: main
