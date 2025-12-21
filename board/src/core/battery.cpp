#include <Arduino.h>
#include "core/battery.hpp"
#include "constants.hpp"

namespace battery {    
    BatteryLevel get_battery_level() {
        int analog_value = analogRead(BAT_PIN);
        float voltage_divider_ratio = (BATTERY_R1 + BATTERY_R2) / BATTERY_R2;
        float voltage = (analog_value / static_cast<float>(MAX_ANALOG_READ)) * voltage_divider_ratio * ANALOG_REF_VOLTAGE;
        constexpr float MAX_VOLTAGE = BATTERY_MAX_VOLTAGE;
        constexpr float MIN_VOLTAGE = BATTERY_MIN_VOLTAGE;
        constexpr float VOLTAGE_RANGE = MAX_VOLTAGE - MIN_VOLTAGE;
        constexpr float BATTERY_VOLTAGE_EMPTY = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.25f);
        constexpr float BATTERY_VOLTAGE_LOW = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.5f);
        constexpr float BATTERY_VOLTAGE_MEDIUM = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.75f);
        
        if (voltage < BATTERY_VOLTAGE_EMPTY) {
            return BatteryLevel::BATTERY_EMPTY;
        } else if (voltage < BATTERY_VOLTAGE_LOW) {
            return BatteryLevel::BATTERY_LOW;
        } else if (voltage < BATTERY_VOLTAGE_MEDIUM) {
            return BatteryLevel::BATTERY_MEDIUM;
        } else {
            return BatteryLevel::BATTERY_FULL;
        }
    }
}