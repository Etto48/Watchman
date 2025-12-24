#include <Arduino.h>

#include "timekeeper.hpp"

namespace timekeeper {
    RTC_DATA_ATTR timeval first_boot_time = {0, 0};
    RTC_DATA_ATTR timeval most_recent_wakeup_time = {0, 0};

    uint64_t now_us() {
        uint64_t delta_s = most_recent_wakeup_time.tv_sec - first_boot_time.tv_sec;
        uint64_t delta_us = most_recent_wakeup_time.tv_usec - first_boot_time.tv_usec;
        return esp_timer_get_time() + delta_s * 1000000 + delta_us;
    }

    void first_boot() {
        gettimeofday(&first_boot_time, nullptr);
        most_recent_wakeup_time = first_boot_time;
    }

    void wakeup() {
        gettimeofday(&most_recent_wakeup_time, nullptr);
    }
}