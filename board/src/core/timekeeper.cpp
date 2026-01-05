#include <Arduino.h>

#include "core/timekeeper.hpp"
#include "apps/settings.hpp"

namespace timekeeper {
    RTC_DATA_ATTR static uint64_t accumulated_time_us = 0;

    uint64_t now_us() {
        return esp_timer_get_time() + accumulated_time_us;
    }

    time_t rtc_s() {
        struct tm timeinfo = {0};
        getLocalTime(&timeinfo, 0);
        if (timeinfo.tm_year + 1900 < 2025) {
            return 0; // RTC not synced
        }
        return mktime(&timeinfo);
    }

    void first_boot() {
        accumulated_time_us = 0;
        auto settings = apps::settings::get_settings();
        update_ntp_settings(settings.timezone);
    }

    void wakeup() {
        auto settings = apps::settings::get_settings();
        update_ntp_settings(settings.timezone);
    }

    void deepsleep() {
        accumulated_time_us = now_us();
    }
    
    void set_time_from_tm(const tm &timeinfo) {
        struct timeval tv;
        time_t t = mktime(const_cast<tm*>(&timeinfo));
        tv.tv_sec = t;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
    }

    void update_ntp_settings(apps::settings::Timezone timezone) {
        auto utc_offset = apps::settings::get_timezone_offset_seconds(timezone);
        auto daylight_offset = apps::settings::get_timezone_daylight_offset_seconds(timezone);
        configTime(utc_offset, daylight_offset, "pool.ntp.org", "time.nist.gov");
    }
}