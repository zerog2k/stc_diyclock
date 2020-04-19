uart.setup(1, 9600, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
uart.write(1, "nodemcu loaded\n")

station_cfg={}
station_cfg.ssid="nautilus" -- SSID of your WiFi network
station_cfg.pwd="xxxxxxxxxx"  -- password of your WiFi network
station_cfg.auto=true
station_cfg.save=true
-- set your static network parameters here
net_ip     = "10.10.10.110"
net_mask   = "255.255.255.0"
net_gate   = "10.10.10.1"
ntp_server = "pool.ntp.org"

timestring = 0
uarttimestring = 0
rtcGood = 0
-- UTCTZ = 5                   -- your timezone (unix TZ) offset
sync_delay = 480            -- sync once per X mins (480 == 8hrs)
sync_att = sync_delay
                        
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
      if (usec > 150000)  then
          while (sec == rtctime.get()) do end
      end
      repeat
          sec, usec = rtctime.get()    
      until usec > 150000
    end
    -- tm = rtctime.epoch2cal((rtctime.get()+1)+UTCTZ*3600)
    tm = rtctime.epoch2cal(rtctime.get())
    timestring = string.format("%d %04d-%02d-%02d %02d:%02d:%02d", tm["wday"], tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
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
    UpdateRtc()
    if (rtcGood == 1) then
        if (sync_att == sync_delay) then
            uart.write(1, uarttimestring)
            print("uart sync sent")
            sync_att = 0
        end
        sync_att = sync_att + 1
    end
end

wifi.setmode(wifi.STATION)
wifi.sta.config(station_cfg)

wifi.sta.setip({ip=net_ip,netmask=net_mask,gateway=net_gate})

print("ESP8266 mode is: " .. wifi.getmode())
print(wifi.sta.getip())
 
if (srv) then
    srv:close()
    srv=nil
end

srv=net.createServer(net.TCP, 4)
print("Server created on " .. wifi.sta.getip())

function receiver(sck, request)
    local response = {}
    -- if you're sending back HTML over HTTP you'll want something like this instead
    -- local response = {"HTTP/1.0 200 OK\r\nServer: NodeMCU on ESP8266\r\nContent-Type: text/html\r\n\r\n"}
    response[#response + 1] = "HTTP/1.1 200 OK\r\n"
    response[#response + 1] = "Content-type: text/html\r\n"
    response[#response + 1] = "Connection: close\r\n\r\n"
    response[#response + 1] = "<html><head>"
    response[#response + 1] = '<title>Clock</title>'
    response[#response + 1] = '<meta http-equiv="refresh" content="5">'
    response[#response + 1] = "<style>"
    response[#response + 1] = 'body {width:100% font-size:52px}'
    response[#response + 1] = 'h1 {font-size:32px}'
    response[#response + 1] = 'button {font-size:42pxheight:4emwidth:30%}'
    response[#response + 1] = "</style>"
    response[#response + 1] = "</head><body>"
    response[#response + 1] = "<h1>Clock</h1>"
    response[#response + 1] = '<div class="container">'
    response[#response + 1] = "<p>Time RTC: ".. timestring .." </p>"
    response[#response + 1] = "<p>NMEA: ".. uarttimestring .." </p>"
    response[#response + 1] = "<p>sync: ".. rtcGood .." </p>"

    response[#response + 1] = '</div>'
    response[#response + 1] = "</body></html>"

    -- sends and removes the first element from the 'response' table
    local function send(localSocket)
        if (#response > 0) then
            localSocket:send(table.remove(response, 1))
        else
            localSocket:close()
            response = nil
        end
    end

    -- triggers the send() function again once the first chunk of data was sent
    sck:on("sent", send)

    send(sck)
end

srv:listen(80, function(conn)
  conn:on("receive", receiver)
end)


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

print("starting sync")

-- sync time / repeat after 1000 sec
sntp.sync(ntp_server, ntpSyncGood, ntpSyncFail, 1 )

uartTimer = tmr.create()
uartTimer:register(10000, tmr.ALARM_AUTO, PrintUart)
uartTimer:interval(60000) -- Maximum value is 6870947 (1:54:30.947).
uartTimer:start()
