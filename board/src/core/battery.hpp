#pragma once

namespace battery {
    enum class BatteryLevel : unsigned int {
        BATTERY_EMPTY = 0,
        BATTERY_LOW = 1,
        BATTERY_MEDIUM = 2,
        BATTERY_FULL = 3,
        BATTERY_DISCONNECTED = 4
    };

    BatteryLevel get_battery_level();
}