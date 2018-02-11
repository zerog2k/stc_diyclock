#include <stdlib.h>

/// super handy website for decoding and understanding nmea format:
// http://freenmea.net

/// line example, randomly generated:
// $GPRMC,224843.541,V,3854.831,N,08102.594,W,48.1,2.91,141017,,E*4E

#define GPSTOKEN_LEN 7
#define GPS_TIMESTAMP_LEN 6
__code const char * GPSTOKEN = "$GPRMC,"; // glonass could be $GL instead of $GP?

typedef struct {
    uint8_t ten_hours;
    uint8_t one_hours;
    uint8_t ten_minutes;
    uint8_t one_minutes;
    uint8_t ten_seconds;
    uint8_t one_seconds;
} gps_time_bcd_t;

int8_t parse_gps_time(const char *tsbuf, gps_time_bcd_t *gpstime)
{
    /// parses gps timestamp string, HHMMSS, to gps_time_bcd_t struct
    // TODO: error checking, safety
    uint8_t i = 0;
    gpstime->ten_hours = (tsbuf[i++] - 0x30);
    gpstime->one_hours = (tsbuf[i++] - 0x30);
    gpstime->ten_minutes = (tsbuf[i++] - 0x30);
    gpstime->one_minutes = (tsbuf[i++] - 0x30);
    gpstime->ten_seconds = (tsbuf[i++] - 0x30);
    gpstime->one_seconds = (tsbuf[i++] - 0x30);
    return i;
}