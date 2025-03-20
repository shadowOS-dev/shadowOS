#ifndef DEV_TIME_RTC_H
#define DEV_TIME_RTC_H

#include <stdint.h>

// RTC Ports
#define RTC_IO_INDEX 0x70
#define RTC_IO_DATA 0x71

// RTC Registers
#define RTC_REG_SECONDS 0x00
#define RTC_REG_MINUTES 0x02
#define RTC_REG_HOURS 0x04
#define RTC_REG_WEEKDAY 0x06
#define RTC_REG_DAY 0x07
#define RTC_REG_MONTH 0x08
#define RTC_REG_YEAR 0x09
#define RTC_REG_CENTURY 0x32 // Some BIOSes use this for century

#define RTC_REG_STATUS_A 0x0A
#define RTC_REG_STATUS_B 0x0B

// Days in each month (non-leap year)
#define RTC_DAYS_IN_MONTHS {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}

// Leap year check
#define RTC_IS_LEAP_YEAR(y) ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))

// Convert RTC time to UNIX timestamp
#define RTC_TO_UNIX_TIMESTAMP(year, month, day, hours, minutes, seconds) ({            \
    static const uint16_t days_in_months[] = RTC_DAYS_IN_MONTHS;                       \
    uint32_t timestamp = 0;                                                            \
    for (uint16_t y = 1970; y < (year); y++)                                           \
    {                                                                                  \
        timestamp += RTC_IS_LEAP_YEAR(y) ? 366 : 365;                                  \
    }                                                                                  \
    for (uint8_t m = 1; m < (month); m++)                                              \
    {                                                                                  \
        timestamp += days_in_months[m] + ((m == 2 && RTC_IS_LEAP_YEAR(year)) ? 1 : 0); \
    }                                                                                  \
    timestamp += (day - 1);                                                            \
    timestamp = (timestamp * 86400) + (hours * 3600) + (minutes * 60) + seconds;       \
    timestamp;                                                                         \
})

#define GET_CURRENT_UNIX_TIME() ({                                    \
    uint8_t hours, minutes, seconds;                                  \
    uint8_t day, month;                                               \
    uint16_t year;                                                    \
    rtc_read_time(&hours, &minutes, &seconds);                        \
    rtc_read_date(&day, &month, &year);                               \
    RTC_TO_UNIX_TIMESTAMP(year, month, day, hours, minutes, seconds); \
})

void rtc_init(void);
void rtc_read_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
void rtc_read_date(uint8_t *day, uint8_t *month, uint16_t *year);

#endif // DEV_TIME_RTC_H
