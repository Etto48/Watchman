#pragma once

#include <cstdint>
#include "apps/settings.hpp"

namespace timekeeper {
    // Returns the current time in microseconds since first boot
    uint64_t now_us();

    // Returns the current time in seconds since the RTC epoch, 0 if RTC not synced
    time_t rtc_s();

    // To be called on first boot
    void first_boot();

    // To be called on wakeup from deep sleep (not on first boot)
    void wakeup();

    // To be called just before entering deep sleep
    void deepsleep();

    // Sets the RTC time from a tm struct
    void set_time_from_tm(const tm& timeinfo);

    // Updates NTP settings based on the given timezone
    void update_ntp_settings(apps::settings::Timezone timezone);
}