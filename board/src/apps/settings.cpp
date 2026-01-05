#include <Preferences.h>
#include <nvs_flash.h>

#include "apps/settings.hpp"
#include "core/menu.hpp"
#include "core/logger.hpp"
#include "core/events.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"



namespace apps::settings {
    menu::KBStatus kb_status;
    
    SemaphoreHandle_t settings_memory_mutex = xSemaphoreCreateMutex();

    static Settings settings;
    bool settings_loaded = false;
    tm base_time = {0};
    enum class TimeSelection : uint8_t {
        YEAR = 0,
        MONTH = 1,
        DAY = 2,
        HOUR = 3,
        MINUTE = 4,
        SECOND = 5,
    };
    TimeSelection time_selection = TimeSelection::YEAR;

    const char* settings_names[] = {
        "Date & Time",
        "Timezone",
        "WiFi Enabled",
        "WiFi SSID",
        "WiFi Password",
        "Location",
        "Save Settings",
        "Reset to Defaults",
        "Factory Reset",
    };

    const char* timezone_names[] = {
        "AoE UTC-12",
        "SST UTC-11",
        "HST UTC-10",
        "MIT UTC-9:30",
        "AKST UTC-9",
        "PST UTC-8",
        "MST UTC-7",
        "CST UTC-6",
        "EST UTC-5",
        "AST UTC-4",
        "NST UTC-3:30",
        "BRT UTC-3",
        "GST UTC-2",
        "AZOT UTC-1",
        "UTC UTC+0",
        "CET UTC+1",
        "CEST UTC+2",
        "MSK UTC+3",
        "IRST UTC+3:30",
        "AZT UTC+4",
        "AFT UTC+4:30",
        "PKT UTC+5",
        "IST UTC+5:30",
        "NPT UTC+5:45",
        "BST UTC+6",
        "MMT UTC+6:30",
        "ICT UTC+7",
        "AWST UTC+8",
        "ACWST UTC+8:45",
        "JST UTC+9",
        "ACST UTC+9:30",
        "AEST UTC+10",
        "LHST UTC+10:30",
        "AEDT UTC+11",
        "NZST UTC+12",
        "CHAST UTC+12:45",
        "TOT UTC+13",
        "LINT UTC+14",
    };

    enum class SettingsOption {
        NONE = 0,
        DATE_TIME = 1,
        TIMEZONE = 2,
        WIFI_ENABLED = 3,
        WIFI_SSID = 4,
        WIFI_PASSWORD = 5,
        LOCATION = 6,
        SAVE_SETTINGS = 7,
        RESET_TO_DEFAULTS = 8,
        FACTORY_RESET = 9,
    };
    SettingsOption current_option = SettingsOption::NONE;
    size_t cursor = 0;
    size_t tz_cursor = 0;

    static Settings new_settings;
    bool new_settings_dirty = false;
    bool new_settings_loaded = false;

    void load_new_settings() {
        if (new_settings_loaded) {
            return;
        }
        new_settings = get_settings();
        new_settings_loaded = true;
        new_settings_dirty = false;
    }

    void action(size_t cursor, Adafruit_SSD1306& display) {
        switch (static_cast<SettingsOption>(cursor + 1)) {
            case SettingsOption::DATE_TIME:
                getLocalTime(&base_time, 0);
                current_option = SettingsOption::DATE_TIME;
                break;
            case SettingsOption::TIMEZONE:
                load_new_settings();
                tz_cursor = static_cast<size_t>(new_settings.timezone);
                current_option = SettingsOption::TIMEZONE;
                break;
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

    void change_time_selection(bool forward) {
        int8_t delta = forward ? 1 : -1;
        int8_t new_selection = static_cast<int8_t>(time_selection) + static_cast<int8_t>(TimeSelection::SECOND) + 1 + delta;
        new_selection %= (static_cast<int8_t>(TimeSelection::SECOND) + 1);
        time_selection = static_cast<TimeSelection>(new_selection);
    }

    void change_time_value(bool up) {
        int64_t delta = up ? 1 : -1;
        tm temp_time = base_time;
        switch (time_selection) {
            case TimeSelection::YEAR:
                temp_time.tm_year += delta;
                break;
            case TimeSelection::MONTH:
                temp_time.tm_mon += delta;
                break;
            case TimeSelection::DAY:
                temp_time.tm_mday += delta;
                break;
            case TimeSelection::HOUR:
                temp_time.tm_hour += delta;
                break;
            case TimeSelection::MINUTE:
                temp_time.tm_min += delta;
                break;
            case TimeSelection::SECOND:
                temp_time.tm_sec += delta;
                break;
        }
        // Normalize time
        time_t normalized = mktime(&temp_time);
        time_t epoch = 0;
        if (normalized == -1) {
            localtime_r(&epoch, &base_time);
        }
        base_time = temp_time;
    }

    void back_action(Adafruit_SSD1306& display) {
        menu::current_app = menu::App::NONE;
    }

    void tz_action(size_t cursor, Adafruit_SSD1306& display) {
        auto new_timezone = static_cast<Timezone>(cursor);
        if (new_timezone != new_settings.timezone) {
            new_settings.timezone = new_timezone;
            new_settings_dirty = true;
        }
        current_option = SettingsOption::NONE;
    }

    void tz_back_action(Adafruit_SSD1306& display) {
        current_option = SettingsOption::NONE;
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
                    action,
                    back_action
                );
                if (ev.type == events::EventType::BUTTON_PRESS &&
                    ev.button_press_event.button == events::Button::B) {
                    // Exiting settings, reset new settings state
                    new_settings_loaded = false;
                    new_settings_dirty = false;
                }
                break;
            case SettingsOption::DATE_TIME:
            {
                if (ev.type == events::EventType::BUTTON_PRESS) {
                    switch (ev.button_press_event.button) {
                        case events::Button::A:
                            sound::play_confirm_tone();
                            timekeeper::set_time_from_tm(base_time);
                            menu::set_dirty();
                            break;
                        case events::Button::UP:
                            sound::play_navigation_tone();
                            change_time_value(true);
                            menu::set_dirty();
                            break;
                        case events::Button::DOWN:
                            sound::play_navigation_tone();
                            change_time_value(false);
                            menu::set_dirty();
                            break;
                        case events::Button::LEFT:
                            sound::play_navigation_tone();
                            change_time_selection(false);
                            menu::set_dirty();
                            break;
                        case events::Button::RIGHT:
                            sound::play_navigation_tone();
                            change_time_selection(true);
                            menu::set_dirty();
                            break;
                        case events::Button::B:
                            sound::play_cancel_tone();
                            current_option = SettingsOption::NONE;
                            menu::set_dirty();
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
            case SettingsOption::TIMEZONE:
            {
                menu::handle_generic_menu_navigation(
                    ev,
                    sizeof(timezone_names)/sizeof(timezone_names[0]),
                    tz_cursor,
                    display,
                    tz_action,
                    tz_back_action
                );
                break;
            }
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
                break;
            }
            case SettingsOption::DATE_TIME:
            {
                display.clearDisplay();
                menu::draw_generic_titlebar(display, "Date & Time");
                display.setTextColor(SSD1306_INVERSE);
                switch (time_selection) {
                    case TimeSelection::YEAR:
                        display.fillRect(9, 19, 4*6 + 1, 9, SSD1306_WHITE);
                        break;
                    case TimeSelection::MONTH:
                        display.fillRect(9 + 5*6, 19, 2*6 + 1, 9, SSD1306_WHITE);
                        break;
                    case TimeSelection::DAY:
                        display.fillRect(9 + 8*6, 19, 2*6 + 1, 9, SSD1306_WHITE);
                        break;
                    case TimeSelection::HOUR:
                        display.fillRect(9, 19+10, 2*6 + 1, 9, SSD1306_WHITE);
                        break;
                    case TimeSelection::MINUTE:
                        display.fillRect(9 + 3*6, 19+10, 2*6 + 1, 9, SSD1306_WHITE);
                        break;
                    case TimeSelection::SECOND:
                        display.fillRect(9 + 6*6, 19+10, 2*6 + 1, 9, SSD1306_WHITE);
                        break;
                }
                display.setCursor(10, 20);
                display.printf(
                    "%04d-%02d-%02d",
                    base_time.tm_year + 1900,
                    base_time.tm_mon + 1,
                    base_time.tm_mday
                );
                display.setCursor(10, 30);
                display.printf(
                    "%02d:%02d:%02d",
                    base_time.tm_hour,
                    base_time.tm_min,
                    base_time.tm_sec
                );
                display.setCursor(10, 50);
                display.print("Press A to set");
                display.display();
                break;
            }
            case SettingsOption::TIMEZONE:
                menu::draw_generic_menu(
                    display,
                    "Timezone",
                    timezone_names,
                    sizeof(timezone_names)/sizeof(timezone_names[0]),
                    tz_cursor
                );
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

    uint64_t get_timezone_offset_seconds(Timezone timezone) {
        switch (timezone) {
            case Timezone::TZ_AoE: return -12 * 3600;
            case Timezone::TZ_SST: return -11 * 3600;
            case Timezone::TZ_HST: return -10 * 3600;
            case Timezone::TZ_MIT: return -9 * 3600 - 1800;
            case Timezone::TZ_AKST: return -9 * 3600;
            case Timezone::TZ_PST: return -8 * 3600;
            case Timezone::TZ_MST: return -7 * 3600;
            case Timezone::TZ_CST: return -6 * 3600;
            case Timezone::TZ_EST: return -5 * 3600;
            case Timezone::TZ_AST: return -4 * 3600;
            case Timezone::TZ_NST: return -3 * 3600 - 1800;
            case Timezone::TZ_BRT: return -3 * 3600;
            case Timezone::TZ_GST: return -2 * 3600;
            case Timezone::TZ_AZOT: return -1 * 3600;
            case Timezone::TZ_UTC: return 0;
            case Timezone::TZ_CET: return 1 * 3600;
            case Timezone::TZ_CEST: return 2 * 3600;
            case Timezone::TZ_MSK: return 3 * 3600;
            case Timezone::TZ_IRST: return 3 * 3600 + 1800;
            case Timezone::TZ_AZT: return 4 * 3600;
            case Timezone::TZ_AFT: return 4 * 3600 + 1800;
            case Timezone::TZ_PKT: return 5 * 3600;
            case Timezone::TZ_IST: return 5 * 3600 + 1800;
            case Timezone::TZ_NPT: return 5 * 3600 + 2700;
            case Timezone::TZ_BST: return 6 * 3600;
            case Timezone::TZ_MMT: return 6 * 3600 + 1800;
            case Timezone::TZ_ICT: return 7 * 3600;
            case Timezone::TZ_AWST: return 8 * 3600;
            case Timezone::TZ_ACWST: return 8 * 3600 + 2700;
            case Timezone::TZ_JST: return 9 * 3600;
            case Timezone::TZ_ACST: return 9 * 3600 + 1800;
            case Timezone::TZ_AEST: return 10 * 3600;
            case Timezone::TZ_LHST: return 10 * 3600 + 1800;
            case Timezone::TZ_AEDT: return 11 * 3600;
            case Timezone::TZ_NZST: return 12 * 3600;
            case Timezone::TZ_CHAST: return 12 * 3600 + 2700;
            case Timezone::TZ_TOT: return 13 * 3600;
            case Timezone::TZ_LINT: return 14 * 3600;
            default: return 0;
        }
    }

    uint64_t get_timezone_daylight_offset_seconds(Timezone timezone) {
        switch (timezone) {
            case Timezone::TZ_CET:
            case Timezone::TZ_CEST:
            case Timezone::TZ_AEST:
            case Timezone::TZ_AEDT:
                return DAYLIGHT_OFFSET_S;
            default:
                return 0;
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
        prefs.putUInt("timezone", static_cast<uint32_t>(new_settings.timezone));
        prefs.putBool("wifi_enabled", new_settings.wifi_enabled);
        prefs.putString("wifi_ssid", new_settings.wifi_ssid);
        prefs.putString("wifi_password", new_settings.wifi_password);
        prefs.putString("location", new_settings.location);
        prefs.end();
        settings_loaded = false; // Force reload next time
        xSemaphoreGive(settings_memory_mutex);
        timekeeper::update_ntp_settings(new_settings.timezone);
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
        settings.timezone = static_cast<Timezone>(prefs.getUInt("timezone", static_cast<uint32_t>(settings.timezone)));
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