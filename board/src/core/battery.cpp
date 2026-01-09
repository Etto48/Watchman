#include <Arduino.h>
#include "core/battery.hpp"
#include "constants.hpp"

namespace battery {    
    BatteryStatus get_battery_status() {
        int analog_value = analogRead(BAT_PIN);
        float voltage_divider_ratio = (BATTERY_R1 + BATTERY_R2) / BATTERY_R2;
        float voltage = (analog_value / static_cast<float>(MAX_ANALOG_READ)) * voltage_divider_ratio * ANALOG_REF_VOLTAGE;
        constexpr float MAX_VOLTAGE = BATTERY_MAX_VOLTAGE;
        constexpr float MIN_VOLTAGE = BATTERY_MIN_VOLTAGE;
        constexpr float VOLTAGE_RANGE = MAX_VOLTAGE - MIN_VOLTAGE;
        constexpr float BATTERY_VOLTAGE_EMPTY = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.25f);
        constexpr float BATTERY_VOLTAGE_LOW = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.5f);
        constexpr float BATTERY_VOLTAGE_MEDIUM = MIN_VOLTAGE + (VOLTAGE_RANGE * 0.75f);
        BatteryLevel level; 
        if (voltage < BATTERY_DISCONNECTED_VOLTAGE) {
            level = BatteryLevel::BATTERY_DISCONNECTED;
        } else if (voltage < BATTERY_VOLTAGE_EMPTY) {
            level = BatteryLevel::BATTERY_EMPTY;
        } else if (voltage < BATTERY_VOLTAGE_LOW) {
            level = BatteryLevel::BATTERY_LOW;
        } else if (voltage < BATTERY_VOLTAGE_MEDIUM) {
            level = BatteryLevel::BATTERY_MEDIUM;
        } else if (voltage < BATTERY_CHARGE_VOLTAGE) {
            level = BatteryLevel::BATTERY_FULL;
        } else {
            level = BatteryLevel::BATTERY_CHARGING;
        }
        uint8_t voltage_dv = static_cast<uint8_t>(voltage * 10); // Convert voltage to decivolts
        return BatteryStatus{level, voltage_dv};
    }
}