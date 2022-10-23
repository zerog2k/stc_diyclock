@echo off
echo "make clean"
del main.hex
del build\*.?*
echo "make"
rem mkdir -p build/
if not exist build mkdir build
SET SDCC_FLAGS=--code-size 4089 --xram-size 0 --data-loc 0x30 
rem do not disable these warnings: --disable-warning 126 --disable-warning 59
SET SDCC_DEFINES=-Dstc15f204ea -DWITH_ALT_LED9 -DWITHOUT_LEDTABLE_RELOC -DSHOW_TEMP_DATE_WEEKDAY
sdcc %SDCC_FLAGS% -Dstc15f204ea  -o build\adc.rel    -c src\adc.c
sdcc %SDCC_FLAGS% -Dstc15f204ea  -o build\ds1302.rel -c src\ds1302.c
sdcc %SDCC_FLAGS% %SDCC_DEFINES% -o build\ src\main.c build\adc.rel build\ds1302.rel
rem tail -n 5 build/main.mem | head -n 2
rem tail -n 1 build/main.mem
copy build\main.ihx main.hex
echo "make flash"
echo stcgal/stcgal.py -p /dev/ttyUSB0 -P auto -t 11059  main.hex
