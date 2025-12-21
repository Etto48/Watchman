#pragma once
#include <cstdint>

constexpr uint64_t SCREEN_WIDTH = 128; // OLED display width, in pixels
constexpr uint64_t SCREEN_HEIGHT = 64; // OLED display height, in pixels
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint32_t MONITOR_SPEED = 115200;

constexpr uint8_t A_PIN = 0;
constexpr uint8_t B_PIN = 6;
constexpr uint8_t UP_PIN = 10;
constexpr uint8_t DOWN_PIN = 9;
constexpr uint8_t LEFT_PIN = 2;
constexpr uint8_t RIGHT_PIN = 5;

constexpr uint8_t TMP_PIN = 1;
constexpr uint8_t BAT_PIN = 3;
constexpr uint8_t BUZZER_PIN = 7;

constexpr uint8_t TXD_PIN = 20;
constexpr uint8_t RXD_PIN = 21;

constexpr uint8_t SDA_PIN = 8;
constexpr uint8_t SCL_PIN = 4;

constexpr uint8_t EVENT_QUEUE_SIZE = 16;
constexpr uint64_t DEBOUNCE_DELAY_MS = 300;

constexpr uint64_t TIME_BEFORE_DEEPSLEEP_MS = 60000; // 1 minute of inactivity before going to deep sleep

constexpr float BATTERY_MAX_VOLTAGE = 4.2f; // Maximum battery voltage
constexpr float BATTERY_MIN_VOLTAGE = 3.0f; // Minimum battery voltage
constexpr float BATTERY_R1 = 100e+3f; // Resistor R1 value in ohms
constexpr float BATTERY_R2 = 100e+3f;  // Resistor R2

constexpr uint16_t MAX_ANALOG_READ = 4095; // 12-bit ADC
constexpr float ANALOG_REF_VOLTAGE = 3.3f; // Reference voltage for ADC

constexpr size_t GMT_OFFSET_S = 3600; // GMT+1
constexpr size_t DAYLIGHT_OFFSET_S = 3600; // Daylight saving time offset