#pragma once

namespace battery {
    enum class BatteryLevel : unsigned int {
        BATTERY_EMPTY = 0,
        BATTERY_LOW = 1,
        BATTERY_MEDIUM = 2,
        BATTERY_FULL = 3,
        BATTERY_CHARGING = 4,
        BATTERY_DISCONNECTED = 5
    };

    struct BatteryStatus {
        BatteryLevel level;
        uint8_t voltage_dv; // battery voltage in decivolts
    };

    BatteryStatus get_battery_status();
}