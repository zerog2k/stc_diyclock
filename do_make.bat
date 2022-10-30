@echo off
echo make clean
del main.hex
del build\*.?*
echo make
rem mkdir -p build/
if not exist build mkdir build

SET SDCC_FLAGS=-Dstc15f204ea --code-size 4089 --xram-size 0 --data-loc 0x30
rem SET SDCC_FLAGS=-Dstc15w404as --code-size 4089 --xram-size 256 --data-loc 0x30
rem SET SDCC_FLAGS=-Dstc15w408as --code-size 8185 --xram-size 256 --data-loc 0x30

rem We do NOT disable these warnings globally: --disable-warning 126 --disable-warning 59
SET MAIN_FLAGS=%SDCC_FLAGS% -DSHOW_TEMP_DATE_WEEKDAY
sdcc %SDCC_FLAGS% -o build\adc.rel    -c src\adc.c
sdcc %SDCC_FLAGS% -o build\ds1302.rel -c src\ds1302.c
sdcc %MAIN_FLAGS% -o build\  src\main.c build\adc.rel build\ds1302.rel
rem tail -n 5 build/main.mem | head -n 2
rem tail -n 1 build/main.mem
copy build\main.ihx main.hex
echo make flash
echo stcgal -p /dev/ttyUSB0 -P auto -t 11059  main.hex
