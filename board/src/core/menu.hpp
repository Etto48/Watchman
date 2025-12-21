#pragma once

#include <Adafruit_SSD1306.h>

namespace menu {
    enum class App {
        NONE = 0,
        TEMPERATURE,
        TIME,
        MUSIC
    };
    
    // Request to redraw the display
    void set_dirty();
    
    // Currently active application, or NONE if in main menu
    extern App current_app;
    
    // Main loop of the system, call this once inside Arduino's loop()
    void main_loop(Adafruit_SSD1306& display);

    // Initialize the menu system and related tasks
    void init();

    // This function is called when there is no event to process
    void upkeep(Adafruit_SSD1306& display);

    // Draw the WiFi icon based on the last known WiFi status
    void draw_wifi_icon(Adafruit_SSD1306& display);

    // Draw the battery icon based on the last known battery level
    void draw_battery_icon(Adafruit_SSD1306& display);
}