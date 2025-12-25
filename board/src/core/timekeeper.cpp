#include <Arduino.h>

#include "timekeeper.hpp"

namespace timekeeper {
    RTC_DATA_ATTR static uint64_t accumulated_time_us = 0;

    uint64_t now_us() {
        return esp_timer_get_time() + accumulated_time_us;
    }

    void first_boot() {
        accumulated_time_us = 0;
    }

    void wakeup() {
        
    }

    void deepsleep() {
        accumulated_time_us = now_us();
    }
}