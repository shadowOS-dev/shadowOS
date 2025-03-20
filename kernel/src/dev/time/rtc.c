#include <dev/time/rtc.h>
#include <dev/portio.h>
#include <lib/log.h>

static inline void rtc_wait_for_update(void)
{
    outb(RTC_IO_INDEX, RTC_REG_STATUS_A);
    while (inb(RTC_IO_DATA) & 0x80)
        ;
}

static uint8_t rtc_read_register(uint8_t reg)
{
    outb(RTC_IO_INDEX, reg);
    return inb(RTC_IO_DATA);
}

static uint8_t rtc_bcd_to_binary(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}

void rtc_init(void)
{
    trace("initializing rtc");

    uint8_t reg_b = rtc_read_register(RTC_REG_STATUS_B);
    if (!(reg_b & 0x02)) // Bit 1: 24-hour mode
    {
        warning("rtc is in 12-hour mode, consider enabling 24-hour mode");
    }

    trace("rtc initialization complete");
}

// Read current time (HH:MM:SS)
void rtc_read_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    rtc_wait_for_update();
    *seconds = rtc_bcd_to_binary(rtc_read_register(RTC_REG_SECONDS));
    *minutes = rtc_bcd_to_binary(rtc_read_register(RTC_REG_MINUTES));
    *hours = rtc_bcd_to_binary(rtc_read_register(RTC_REG_HOURS));
}

// Read current date (DD/MM/YYYY)
void rtc_read_date(uint8_t *day, uint8_t *month, uint16_t *year)
{
    rtc_wait_for_update();
    *day = rtc_bcd_to_binary(rtc_read_register(RTC_REG_DAY));
    *month = rtc_bcd_to_binary(rtc_read_register(RTC_REG_MONTH));
    *year = rtc_bcd_to_binary(rtc_read_register(RTC_REG_YEAR)) + 2000; // Assuming 21st century
}
