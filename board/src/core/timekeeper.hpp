#pragma once

#include <cstdint>

namespace timekeeper {
    // Returns the current time in microseconds since first boot
    uint64_t now_us();

    // To be called on first boot
    void first_boot();

    // To be called on wakeup from deep sleep (not on first boot)
    void wakeup();
}