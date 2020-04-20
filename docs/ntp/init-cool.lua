uart.setup(1, 9600, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
uart.write(1, "nodemcu loaded\n")

station_cfg = {}
station_cfg.ssid = "RON_G" -- SSID of your WiFi network
station_cfg.pwd = "xxxxxxxxxxxxxx"  -- password of your WiFi network
station_cfg.auto = false
station_cfg.save = true

DHCP = 1
-- set your static network parameters here if DHCP == 0
net_ip     = "10.10.10.110"
net_mask   = "255.255.255.0"
net_gate   = "10.10.10.1"
ntp_server = "pool.ntp.org"

uarttimestring = 0
rtcGood = 0
-- UTCTZ = 5                   -- your timezone (unix TZ) offset
sync_att = 0

local function check_int(n)
    -- checking not float
    if (n - math.floor(n) > 0) then
        error("trying to use bitwise operation on non-integer!")
    end
end

local function to_bits(n)
    check_int(n)
    if (n < 0) then
        -- negative
        return to_bits(bit.bnot(math.abs(n)) + 1)
    end
    -- to bits table
    local tbl = {}
    local cnt = 1
    while (n > 0) do
        local last = n % 2
        if (last == 1) then
            tbl[cnt] = 1
        else
            tbl[cnt] = 0
        end
        n = (n-last)/2
        cnt = cnt + 1
    end
    return tbl
end

local function tbl_to_number(tbl)
    local n = table.getn(tbl)
    local rslt = 0
    local power = 1
    for i = 1, n do
        rslt = rslt + tbl[i]*power
        power = power*2
    end
    return rslt
end

local function expand(tbl_m, tbl_n)
    local big = {}
    local small = {}
    if (table.getn(tbl_m) > table.getn(tbl_n)) then
        big = tbl_m
        small = tbl_n
    else
        big = tbl_n
        small = tbl_m
    end
    -- expand small
    for i = table.getn(small) + 1, table.getn(big) do
        small[i] = 0
    end
end

local function bit_xor(m, n)
    local tbl_m = to_bits(m)
    local tbl_n = to_bits(n)
    expand(tbl_m, tbl_n)
    local tbl = {}
    local rslt = math.max(table.getn(tbl_m), table.getn(tbl_n))
    for i = 1, rslt do
        if (tbl_m[i] ~= tbl_n[i]) then
            tbl[i] = 1
        else
            tbl[i] = 0
        end
    end
    --table.foreach(tbl, print)
    return tbl_to_number(tbl)
end

local function to_hex(n) 
    if (type(n) ~= "number") then
        error("non-number type passed in.")
    end

    -- checking not float
    if (n - math.floor(n) > 0) then
        error("trying to apply bitwise operation on non-integer!")
    end

    if (n < 0) then
        -- negative
        n = bit.tobits(bit.bnot(math.abs(n)) + 1)
        n = bit.tonumb(n)
    end

    hex_tbl = {'A', 'B', 'C', 'D', 'E', 'F'}
    hex_str = ""

    while (n ~= 0) do
        last = n % 16
        if (last < 10) then
            hex_str = tostring(last) .. hex_str
        else
            hex_str = hex_tbl[last-10+1] .. hex_str
        end
        n = math.floor(n/16)
    end
    if (hex_str == "") then
        hex_str = "0"
    end
    if (string.len(hex_str) < 2) then
        return "0"..hex_str
    else
        return hex_str
    end
end

function UpdateRtc()
    local sec, usec = rtctime.get()
    if (sec ~= 0) then
      while (sec == rtctime.get()) do end
    end
    -- tm = rtctime.epoch2cal((rtctime.get()+1)+UTCTZ*3600)
    tm = rtctime.epoch2cal(rtctime.get())
    if (tm["year"] >= 2000) then
        tm["year"] = tm["year"] - 2000;
    else
        tm["year"] = 0;
    end
    uarttimestring = string.format("$GPRMC,%02d%02d%02d.00,A,,,,,,,%02d%02d%02d,,,*",
                           tm["hour"], tm["min"], tm["sec"], tm["day"], tm["mon"], tm["year"]);    
    local crc = 0
    for i=2,string.len(uarttimestring)-1 do
        crc = bit_xor(crc, string.byte(uarttimestring,i));
    end
    uarttimestring = uarttimestring..to_hex(crc).."\r\n"
    if (rtcGood == 1) then
        print(uarttimestring)
    else
        print("ntp not synced")
    end
end

function PrintUart()
    print("ESP8266 wifi : " .. wifi.sta.getip())
    UpdateRtc()
    if (rtcGood == 1) then
        uart.write(1, uarttimestring)
        print("uart sync sent")
        sync_att = sync_att + 1
        if (sync_att >= 2) then
            print("going to deep sleep mode")
            rtctime.dsleep(1800000000); -- 1/2 hrs
        end
    end
end

wifi.setmode(wifi.STATION)
wifi.sta.config(station_cfg)
if DHCP == 0 then
    wifi.sta.setip({ip=net_ip,netmask=net_mask,gateway=net_gate})
end

 
function ntpSyncGood(sec, usec, server, info)
    print('ntp synced', sec, usec, server)
    offset_s = info.offset_s or 0
    offset_us = info.offset_us or 0
    delay_us = info.delay_us or 0
    stratum = info.stratum or 0
    leap = info.leap or 0
    print("offset_s "  .. offset_s)
    print("offset_us " .. offset_us)
    print("delay_us " .. delay_us )
    print("stratum " ..  stratum )
    print("leap " ..  leap)
    rtcGood = 1
end

function ntpSyncFail()
    print('failed!')
    rtcGood = 0
end


function wifi_connected()
    print("starting sync")
    sntp.sync(ntp_server, ntpSyncGood, ntpSyncFail, 1 )
end

wifi.sta.connect(wifi_connected)

-- sync time / repeat after 1000 sec

uartTimer = tmr.create()
uartTimer:register(10000, tmr.ALARM_AUTO, PrintUart)
uartTimer:interval(10000) -- Maximum value is 6870947 (1:54:30.947).
uartTimer:start()
