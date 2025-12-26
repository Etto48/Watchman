#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::settings {
    struct Settings {
        bool wifi_enabled = false;
        String wifi_ssid = "";
        String wifi_password = "";
        String location = "Greenwich"; // Default location
    };

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