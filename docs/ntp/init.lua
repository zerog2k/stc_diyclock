
-- serial port at 57600 8N1
uart.setup(1, 57600, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
uart.write(1, "nodemcu loaded\n")

station_cfg={}
station_cfg.ssid="yourssid" -- SSID of your WiFi network
station_cfg.pwd="yourpass"  -- password of your WiFi network
station_cfg.auto=true
station_cfg.save=true

timestring = 0
uarttimestring = 0
rtcGood = 0
UTCTZ = 5                   -- your timezone (unix TZ) offset
sync_delay = 480            -- sync once per X mins (480 == 8hrs)
sync_att = sync_delay

function UpdateRtc()
    prev = rtctime.get()
    while prev == rtctime.get() do
        -- wait until next second started
    end
    tm = rtctime.epoch2cal(rtctime.get()+UTCTZ*3600)
    timestring = string.format("%d %04d-%02d-%02d %02d:%02d:%02d", tm["wday"], tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
    uarttimestring = string.format("#%d%04d%02d%02d%02d%02d%02d#\0\0", tm["wday"], tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
    if rtcGood == 1 then
        print(uarttimestring)
    else
        print("ntp not synced")
    end
end

function PrintUart()
    UpdateRtc()
    if rtcGood == 1 then
        if sync_att == sync_delay then
            uart.write(1, uarttimestring)
            print("uart sync sent")
            sync_att = 0
        end
        sync_att = sync_att + 1
    end
end

wifi.setmode(wifi.STATION)
wifi.sta.config(station_cfg)

-- set your static network parameters here
wifi.sta.setip({ip="10.10.10.110",netmask="255.255.255.0",gateway="10.10.10.1"})

print("ESP8266 mode is: " .. wifi.getmode())
print(wifi.sta.getip())
 
if srv then 
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
    response[#response + 1] = 'h1 {font-size:62px}'
    response[#response + 1] = 'button {font-size:42pxheight:4emwidth:30%}'
    response[#response + 1] = "</style>"
    response[#response + 1] = "</head><body>"
    response[#response + 1] = "<h1>Clock</h1>"
    response[#response + 1] = '<div class="container">'
    response[#response + 1] = "<p>Time RTC: ".. timestring .." </p>"
    response[#response + 1] = "<p>sync: ".. rtcGood .." </p>"

    response[#response + 1] = '</div>'
    response[#response + 1] = "</body></html>"

    -- sends and removes the first element from the 'response' table
    local function send(localSocket)
        if #response > 0 then 
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
sntp.sync("pool.ntp.org", ntpSyncGood, ntpSyncFail, 1 )

uartTimer = tmr.create()
uartTimer:register(5000, tmr.ALARM_AUTO, PrintUart)
uartTimer:interval(60000) -- Maximum value is 6870947 (1:54:30.947).
uartTimer:start()
