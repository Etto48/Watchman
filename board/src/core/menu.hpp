#pragma once

#include <Adafruit_SSD1306.h>

#include "core/events.hpp"

namespace menu {
    enum class App {
        NONE = 0,
        TEMPERATURE = 1,
        CLOCK = 2,
        STOPWATCH = 3,
        TIMER = 4,
        ALARM = 5,
        CALENDAR = 6,
        MUSIC = 7,
        WEATHER = 8,
        PET = 9,
        MAP = 10,
        METRONOME = 11,
        EVENT_DEBUGGER = 12,
        DEEPSLEEP = 13,
        SETTINGS = 14
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
    void upkeep(Adafruit_SSD1306& display, uint64_t delay_ms = 100);

    // Draw the WiFi icon based on the last known WiFi status
    void draw_wifi_icon(Adafruit_SSD1306& display);

    // Draw the battery icon based on the last known battery level
    void draw_battery_icon(Adafruit_SSD1306& display);

    // Helper to draw a title bar
    void draw_generic_titlebar(Adafruit_SSD1306& display, const char* title);

    // Helper to unify drawing of multi-option menus
    void draw_generic_menu(Adafruit_SSD1306& display, const char* title, const char *const options[], size_t option_count, size_t selected_index);

    // Helper to move cursor up in multi-option menus
    void generic_cursor_up(size_t &cursor, size_t option_count);

    // Helper to move cursor down in multi-option menus
    void generic_cursor_down(size_t &cursor, size_t option_count);

    // Helper to unify navigation in multi-option menus
    // Action callback is called when the A button is pressed
    void handle_generic_menu_navigation(
        events::Event ev, 
        size_t option_count, 
        size_t &cursor, 
        Adafruit_SSD1306& display,
        void(*action)(size_t cursor, Adafruit_SSD1306& display), 
        void(*back_action)(Adafruit_SSD1306& display) = nullptr,
        bool play_confirm_tone = true, 
        bool play_cancel_tone = true, 
        bool play_navigation_tone = true);


    struct KBStatus {
        size_t selected_key = 0;
        bool shift_active = false;
        bool caps_active = false;
    };

    enum class KBEvent {
        NONE = 0, // User did nothing or navigated without selecting a key
        ENTERED_CHARACTER,
        DELETED_CHARACTER,
        TAB_PRESSED, // Useful for autocompletion
        ENTER_PRESSED,
        KEYBOARD_CLOSED,
    };

    // Handle keyboard input events, to be used along with menu::draw_keyboard
    // Returns the KBEvent that occurred if any
    KBEvent handle_keyboard_input(
        events::Event ev,
        KBStatus& kb_status,
        String &input_buffer);
    
    // Draw a keyboard interface, to be used along with menu::handle_keyboard_input
    void draw_keyboard(
        Adafruit_SSD1306& display, 
        const KBStatus& kb_status,
        const String& input_buffer);

    enum class BooleanSwitchEvent {
        NONE = 0,
        TOGGLED, // Value was toggled
        CLOSED, // User exited the boolean switch menu
    };

    // Handle boolean switch input events, to be used along with menu::draw_boolean_switch
    // Returns the BooleanSwitchEvent that occurred if any
    BooleanSwitchEvent handle_boolean_switch_input(
        events::Event ev, 
        bool & value, 
        Adafruit_SSD1306& display
    );

    // Draw a boolean switch interface, to be used along with menu::handle_boolean_switch_input
    void draw_boolean_switch(
        Adafruit_SSD1306& display, 
        const char* title, 
        bool value);

    enum class ConfirmationDialogResult {
        NONE = 0,
        CONFIRMED,
        CANCELED
    };

    // Handle confirmation dialog input events, to be used along with menu::draw_confirmation_dialog
    // Returns the ConfirmationDialogResult that occurred if any
    ConfirmationDialogResult handle_confirmation_dialog_input(
        events::Event ev, 
        Adafruit_SSD1306& display
    );

    // Draw a confirmation dialog interface, to be used along with menu::handle_confirmation_dialog_input
    void draw_confirmation_dialog(
        Adafruit_SSD1306& display, 
        const char* message
    );
}