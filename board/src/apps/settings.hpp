#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::settings {
    enum class Timezone {
        TZ_AoE, // Anywhere on Earth (UTC-12)
        TZ_SST, // Samoa Standard Time (UTC-11)
        TZ_HST, // Hawaii Standard Time (UTC-10)
        TZ_MIT, // Marquesas Islands Time (UTC-9:30)
        TZ_AKST, // Alaska Standard Time (UTC-9)
        TZ_PST, // Pacific Standard Time (UTC-8)
        TZ_MST, // Mountain Standard Time (UTC-7)
        TZ_CST, // Central Standard Time (UTC-6)
        TZ_EST, // Eastern Standard Time (UTC-5)
        TZ_AST, // Atlantic Standard Time (UTC-4)
        TZ_NST, // Newfoundland Standard Time (UTC-3:30)
        TZ_BRT, // Brasilia Time (UTC-3)
        TZ_GST, // South Georgia Time (UTC-2)
        TZ_AZOT, // Azores Time (UTC-1)
        TZ_UTC, // Coordinated Universal Time (UTC+0)
        TZ_CET, // Central European Time (UTC+1)
        TZ_CEST, // Eastern European Time (UTC+2)
        TZ_MSK, // Moscow Standard Time (UTC+3)
        TZ_IRST, // Iran Standard Time (UTC+3:30)
        TZ_AZT, // Azerbaijan Time (UTC+4)
        TZ_AFT, // Afghanistan Time (UTC+4:30)
        TZ_PKT, // Pakistan Standard Time (UTC+5)
        TZ_IST, // India Standard Time (UTC+5:30)
        TZ_NPT, // Nepal Time (UTC+5:45)
        TZ_BST, // Bangladesh Standard Time (UTC+6)
        TZ_MMT, // Myanmar Time (UTC+6:30)
        TZ_ICT, // Indochina Time (UTC+7)
        TZ_AWST, // Australian Western Standard Time (UTC+8)
        TZ_ACWST, // Australian Central Western Standard Time (UTC+8:45)
        TZ_JST, // Japan Standard Time (UTC+9)
        TZ_ACST, // Australian Central Standard Time (UTC+9:30)
        TZ_AEST, // Australian Eastern Standard Time (UTC+10)
        TZ_LHST, // Lord Howe Standard Time (UTC+10:30)
        TZ_AEDT, // Australian Eastern Daylight Time (UTC+11)
        TZ_NZST, // New Zealand Standard Time (UTC+12)
        TZ_CHAST, // Chatham Standard Time (UTC+12:45)
        TZ_TOT, // Tonga Time (UTC+13)
        TZ_LINT, // Line Islands Time (UTC+14)
    };

    struct Settings {
        bool wifi_enabled = false;
        String wifi_ssid = "";
        String wifi_password = "";
        String location = "Greenwich"; // Default location
        Timezone timezone = Timezone::TZ_UTC;
    };

    uint64_t get_timezone_offset_seconds(Timezone timezone);
    uint64_t get_timezone_daylight_offset_seconds(Timezone timezone);

    void app(Adafruit_SSD1306& display);
    void draw(Adafruit_SSD1306& display);

    // Save the provided settings to non-volatile storage
    void save_settings(Settings& new_settings);

    // Reset all settings to factory defaults, formatting NVS storage
    void factory_reset();

    // Clear all saved settings from non-volatile storage without formatting
    void clear_settings();

    // Retrieve the current settings from non-volatile storage (or defaults if not set)
    Settings get_settings();
}