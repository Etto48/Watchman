#include <Preferences.h>
#include <nvs_flash.h>

#include "apps/settings.hpp"
#include "core/menu.hpp"
#include "core/logger.hpp"



namespace apps::settings {
    menu::KBStatus kb_status;
    
    SemaphoreHandle_t settings_memory_mutex = xSemaphoreCreateMutex();

    static Settings settings;
    bool settings_loaded = false;

    const char* settings_names[] = {
        "WiFi Enabled",
        "WiFi SSID",
        "WiFi Password",
        "Location",
        "Save Settings",
        "Reset to Defaults",
        "Factory Reset",
    };

    enum class SettingsOption {
        NONE = 0,
        WIFI_ENABLED = 1,
        WIFI_SSID = 2,
        WIFI_PASSWORD = 3,
        LOCATION = 4,
        SAVE_SETTINGS = 5,
        RESET_TO_DEFAULTS = 6,
        FACTORY_RESET = 7,
    };
    SettingsOption current_option = SettingsOption::NONE;
    size_t cursor = 0;

    static Settings new_settings;
    bool new_settings_dirty = false;
    bool new_settings_loaded = false;

    void load_new_settings() {
        if (new_settings_loaded) {
            return;
        }
        new_settings = get_settings();
        new_settings_loaded = true;
    }

    void action(size_t cursor, Adafruit_SSD1306& display) {
        switch (static_cast<SettingsOption>(cursor + 1)) {
            case SettingsOption::WIFI_ENABLED:
                load_new_settings();
                current_option = SettingsOption::WIFI_ENABLED;
                break;
            case SettingsOption::WIFI_SSID:
                load_new_settings();
                current_option = SettingsOption::WIFI_SSID;
                break;
            case SettingsOption::WIFI_PASSWORD:
                load_new_settings();
                current_option = SettingsOption::WIFI_PASSWORD;
                break;
            case SettingsOption::LOCATION:
                load_new_settings();
                current_option = SettingsOption::LOCATION;
                break;
            case SettingsOption::SAVE_SETTINGS:
                load_new_settings();
                current_option = SettingsOption::SAVE_SETTINGS;
                break;
            case SettingsOption::RESET_TO_DEFAULTS:
                current_option = SettingsOption::RESET_TO_DEFAULTS;
                break;
            case SettingsOption::FACTORY_RESET:
                current_option = SettingsOption::FACTORY_RESET;
                break;

            default:
                break;
        }
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (current_option) {
            case SettingsOption::NONE:
                menu::handle_generic_menu_navigation(
                    ev,
                    sizeof(settings_names)/sizeof(settings_names[0]),
                    cursor,
                    display,
                    action
                );
                if (ev.type == events::EventType::BUTTON_PRESS &&
                    ev.button_press_event.button == events::Button::B) {
                    // Exiting settings, reset new settings state
                    new_settings_loaded = false;
                    new_settings_dirty = false;
                }
                break;
            case SettingsOption::WIFI_ENABLED:
            {
                auto switch_event = menu::handle_boolean_switch_input(
                    ev,
                    new_settings.wifi_enabled,
                    display
                );
                if (switch_event == menu::BooleanSwitchEvent::CLOSED) {
                    if (new_settings.wifi_enabled != settings.wifi_enabled) {
                        new_settings_dirty = true;
                    }
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::WIFI_SSID:
            {
                auto kb_event = menu::handle_keyboard_input(
                    ev,
                    kb_status,
                    new_settings.wifi_ssid
                );
                if (kb_event == menu::KBEvent::ENTER_PRESSED) {
                    if (new_settings.wifi_ssid != settings.wifi_ssid) {
                        new_settings_dirty = true;
                    }
                    current_option = SettingsOption::NONE;
                } else if (kb_event == menu::KBEvent::KEYBOARD_CLOSED) {
                    new_settings.wifi_ssid = settings.wifi_ssid; // Revert changes
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::WIFI_PASSWORD:
            {
                auto kb_event = menu::handle_keyboard_input(
                    ev,
                    kb_status,
                    new_settings.wifi_password
                );
                if (kb_event == menu::KBEvent::ENTER_PRESSED) {
                    if (new_settings.wifi_password != settings.wifi_password) {
                        new_settings_dirty = true;
                    }
                    current_option = SettingsOption::NONE;
                } else if (kb_event == menu::KBEvent::KEYBOARD_CLOSED) {
                    new_settings.wifi_password = settings.wifi_password; // Revert changes
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::LOCATION:
            {
                auto kb_event = menu::handle_keyboard_input(
                    ev,
                    kb_status,
                    new_settings.location
                );
                if (kb_event == menu::KBEvent::ENTER_PRESSED) {
                    if (new_settings.location != settings.location) {
                        new_settings_dirty = true;
                    }
                    current_option = SettingsOption::NONE;
                } else if (kb_event == menu::KBEvent::KEYBOARD_CLOSED) {
                    new_settings.location = settings.location; // Revert changes
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::SAVE_SETTINGS:
            {
                auto confirm_event = menu::handle_confirmation_dialog_input(ev, display);
                if (confirm_event == menu::ConfirmationDialogResult::CONFIRMED) {
                    save_settings(new_settings);
                    settings = new_settings;
                    new_settings_dirty = false;
                    current_option = SettingsOption::NONE;
                } else if (confirm_event == menu::ConfirmationDialogResult::CANCELED) {
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::RESET_TO_DEFAULTS:
            {
                auto confirm_event = menu::handle_confirmation_dialog_input(ev, display);
                if (confirm_event == menu::ConfirmationDialogResult::CONFIRMED) {
                    clear_settings();
                    settings = Settings(); // Reset to default constructed settings
                    new_settings = settings;
                    new_settings_dirty = false;
                    current_option = SettingsOption::NONE;
                } else if (confirm_event == menu::ConfirmationDialogResult::CANCELED) {
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            case SettingsOption::FACTORY_RESET:
            {
                auto confirm_event = menu::handle_confirmation_dialog_input(ev, display);
                if (confirm_event == menu::ConfirmationDialogResult::CONFIRMED) {
                    factory_reset();
                    settings = Settings(); // Reset to default constructed settings
                    new_settings = settings;
                    new_settings_dirty = false;
                    current_option = SettingsOption::NONE;
                } else if (confirm_event == menu::ConfirmationDialogResult::CANCELED) {
                    current_option = SettingsOption::NONE;
                }
                break;
            }
            default:
                break;
        }
        if (ev.type == events::EventType::NONE) {
            menu::upkeep(display);
        }
    }

    void draw(Adafruit_SSD1306& display) {
        switch (current_option) {
            case SettingsOption::NONE:
            {
                const char* title = new_settings_dirty ? "Settings *" : "Settings";
                menu::draw_generic_menu(
                    display,
                    title,
                    settings_names,
                    sizeof(settings_names)/sizeof(settings_names[0]),
                    cursor
                );
            }
                break;
            case SettingsOption::WIFI_ENABLED:
                menu::draw_boolean_switch(
                    display,
                    "WiFi Enabled",
                    new_settings.wifi_enabled
                );
                break;
            case SettingsOption::WIFI_SSID:
                menu::draw_keyboard(
                    display,
                    kb_status,
                    new_settings.wifi_ssid
                );
                break;
            case SettingsOption::WIFI_PASSWORD:
                menu::draw_keyboard(
                    display,
                    kb_status,
                    new_settings.wifi_password
                );
                break;
            case SettingsOption::LOCATION:
                menu::draw_keyboard(
                    display,
                    kb_status,
                    new_settings.location
                );
                break;
            case SettingsOption::SAVE_SETTINGS:
                menu::draw_confirmation_dialog(
                    display,
                    "Save Settings?"
                );
                break;
            case SettingsOption::RESET_TO_DEFAULTS:
                menu::draw_confirmation_dialog(
                    display,
                    "Reset to Defaults?"
                );
                break;
            case SettingsOption::FACTORY_RESET:
                menu::draw_confirmation_dialog(
                    display,
                    "Reset the board to\nFactory Settings?"
                );
                break;
            default:
                break;
        }
    }

    void save_settings(Settings& new_settings) {
        xSemaphoreTake(settings_memory_mutex, portMAX_DELAY);
        Preferences prefs;
        if (!prefs.begin("settings", false)) {
            logger::error("Failed to open settings for writing.");
            xSemaphoreGive(settings_memory_mutex);
            return;
        }
        prefs.putBool("wifi_enabled", new_settings.wifi_enabled);
        prefs.putString("wifi_ssid", new_settings.wifi_ssid);
        prefs.putString("wifi_password", new_settings.wifi_password);
        prefs.putString("location", new_settings.location);
        prefs.end();
        settings_loaded = false; // Force reload next time
        xSemaphoreGive(settings_memory_mutex);
    }

    void factory_reset() {
        xSemaphoreTake(settings_memory_mutex, portMAX_DELAY);
        esp_err_t err = nvs_flash_erase();
        if (err != ESP_OK) {
            logger::error("Failed to erase NVS flash: %d", err);
            xSemaphoreGive(settings_memory_mutex);
            return;
        }
        nvs_flash_init();
        logger::info("Settings reset to factory defaults.");
        settings_loaded = false; // Force reload next time
        xSemaphoreGive(settings_memory_mutex);
    }

    void clear_settings() {
        xSemaphoreTake(settings_memory_mutex, portMAX_DELAY);
        Preferences prefs;
        if (!prefs.begin("settings", false)) {
            logger::error("Failed to open settings for clearing.");
            xSemaphoreGive(settings_memory_mutex);
            return;
        }
        prefs.clear();
        prefs.end();
        logger::info("All settings cleared.");
        settings_loaded = false; // Force reload next time
        xSemaphoreGive(settings_memory_mutex);
    }

    Settings get_settings() {
        xSemaphoreTake(settings_memory_mutex, portMAX_DELAY);
        if (settings_loaded) {
            xSemaphoreGive(settings_memory_mutex);
            return settings;
        }
        Preferences prefs;
        if (!prefs.begin("settings", true)) {
            logger::error("Failed to open settings for reading.");
            xSemaphoreGive(settings_memory_mutex);
            return settings;
        }
        settings.wifi_enabled = prefs.getBool("wifi_enabled", settings.wifi_enabled);
        settings.wifi_ssid = prefs.getString("wifi_ssid", settings.wifi_ssid);
        settings.wifi_password = prefs.getString("wifi_password", settings.wifi_password);
        settings.location = prefs.getString("location", settings.location);
        prefs.end();
        settings_loaded = true;
        xSemaphoreGive(settings_memory_mutex);
        return settings;
    }
}