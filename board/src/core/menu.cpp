#include <cstdint>

#include "core/menu.hpp"
#include "core/events.hpp"
#include "core/sound.hpp"
#include "core/image.hpp"
#include "apps/temperature.hpp"
#include "apps/music.hpp"
#include "apps/weather.hpp"
#include "apps/stopwatch.hpp"
#include "apps/timer.hpp"
#include "apps/alarm.hpp"
#include "apps/calendar.hpp"
#include "apps/pet.hpp"
#include "apps/map.hpp"
#include "apps/metronome.hpp"
#include "apps/battery.hpp"
#include "apps/event_debugger.hpp"
#include "apps/deepsleep.hpp"
#include "apps/settings.hpp"
#include "core/deepsleep.hpp"
#include "constants.hpp"
#include "images/wifi_0.hpp"
#include "images/wifi_1.hpp"
#include "images/wifi_2.hpp"
#include "images/wifi_3.hpp"
#include "images/wifi_off.hpp"
#include "images/battery_0.hpp"
#include "images/battery_1.hpp"
#include "images/battery_2.hpp"
#include "images/battery_3.hpp"
#include "images/battery_4.hpp"
#include "images/battery_5.hpp"
#include "images/alarm_0.hpp"
#include "images/alarm_1.hpp"
#include "apps/clock.hpp"
#include "core/wifi.hpp"
#include "core/battery.hpp"
#include "core/menu.hpp"
#include "core/timekeeper.hpp"
#include "menu.hpp"

namespace menu {
    static SemaphoreHandle_t status_mutex = nullptr;
    bool dirty = true;
    wifi::WiFiStatus last_wifi_status = wifi::WiFiStatus::DISCONNECTED;
    battery::BatteryLevel last_battery_level = battery::BatteryLevel::BATTERY_EMPTY;
    bool last_alarm_set = false; // True if an alarm is set
    
    App current_app = App::NONE;
    size_t cursor = 0;

    constexpr const char *menu_items[] = {
        "Temperature",
        "Clock",
        "Stopwatch",
        "Timer",
        "Alarm",
        "Calendar",
        "Music",
        "Weather",
        "Pet",
        "Map",
        "Metronome",
        "Battery",
        "Event Debugger",
        "Deepsleep",
        "Settings"
    };
    constexpr void(*app_mains[])(Adafruit_SSD1306&) = {
        apps::temperature::app,
        apps::clock::app,
        apps::stopwatch::app,
        apps::timer::app,
        apps::alarm::app,
        apps::calendar::app,
        apps::music::app,
        apps::weather::app,
        apps::pet::app,
        apps::map::app,
        apps::metronome::app,
        apps::battery::app,
        apps::event_debugger::app,
        apps::deepsleep::app,
        apps::settings::app
    };
    constexpr void(*app_draws[])(Adafruit_SSD1306&) = {
        apps::temperature::draw,
        apps::clock::draw,
        apps::stopwatch::draw,
        apps::timer::draw,
        apps::alarm::draw,
        apps::calendar::draw,
        apps::music::draw,
        apps::weather::draw,
        apps::pet::draw,
        apps::map::draw,
        apps::metronome::draw,
        apps::battery::draw,
        apps::event_debugger::draw,
        apps::deepsleep::draw,
        apps::settings::draw
    };

    void draw_wifi_icon(Adafruit_SSD1306& display) {
        const uint8_t* wifi_icon = nullptr;
        switch (last_wifi_status) {
            case wifi::WiFiStatus::DISCONNECTED:
                wifi_icon = images::wifi_0;
                break;
            case wifi::WiFiStatus::CONNECTED_WEAK:
                wifi_icon = images::wifi_1;
                break;
            case wifi::WiFiStatus::CONNECTED_AVERAGE:
                wifi_icon = images::wifi_2;
                break;
            case wifi::WiFiStatus::CONNECTED_STRONG:
                wifi_icon = images::wifi_3;
                break;
            case wifi::WiFiStatus::DISABLED_BY_USER:
                wifi_icon = images::wifi_off;
                break;
            default:
                wifi_icon = images::wifi_0;
                break;
        }
        display.drawBitmap(SCREEN_WIDTH - images::wifi_0_width, 0, wifi_icon, images::wifi_0_width, images::wifi_0_height, SSD1306_BLACK);
    }

    void draw_battery_icon(Adafruit_SSD1306 &display) {
        const uint8_t* battery_icon = nullptr;
        switch (last_battery_level) {
            case battery::BatteryLevel::BATTERY_EMPTY:
                battery_icon = images::battery_0;
                break;
            case battery::BatteryLevel::BATTERY_LOW:
                battery_icon = images::battery_1;
                break;
            case battery::BatteryLevel::BATTERY_MEDIUM:
                battery_icon = images::battery_2;
                break;
            case battery::BatteryLevel::BATTERY_FULL:
                battery_icon = images::battery_3;
                break;
            case battery::BatteryLevel::BATTERY_CHARGING:
                battery_icon = images::battery_4;
                break;
            case battery::BatteryLevel::BATTERY_DISCONNECTED:
                battery_icon = images::battery_5;
                break;
            default:
                battery_icon = images::battery_0;
                break;
        }
        display.drawBitmap(SCREEN_WIDTH - images::battery_0_width * 2, 0, battery_icon, images::battery_0_width, images::battery_0_height, SSD1306_BLACK);
    }

    void draw_alarm_icon(Adafruit_SSD1306 &display) {
        if (last_alarm_set) {
            display.drawBitmap(SCREEN_WIDTH - images::battery_0_width * 3, 0, images::alarm_1, images::alarm_1_width, images::alarm_1_height, SSD1306_BLACK);
        } else {
            display.drawBitmap(SCREEN_WIDTH - images::battery_0_width * 3, 0, images::alarm_0, images::alarm_0_width, images::alarm_0_height, SSD1306_BLACK);
        }
    }

    void draw_generic_titlebar(Adafruit_SSD1306 &display, const char *title)
    {
        display.setTextSize(1);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(0, 0);
        display.print(" ");
        display.println(title);
        draw_alarm_icon(display);
        draw_battery_icon(display);
        draw_wifi_icon(display);
        display.setTextColor(SSD1306_WHITE);
    }

    void draw_generic_menu(Adafruit_SSD1306 &display, const char *title, const char *const options[], size_t option_count, size_t selected_index)
    {
        display.clearDisplay();
        draw_generic_titlebar(display, title);
        size_t start_index = 0;
        size_t end_index = option_count; // Exclusive
        bool can_scroll_up = false;
        bool can_scroll_down = false;
        if (option_count > 7) { // Only 7 options can fit on screen at once
            if (selected_index < 4) {
                start_index = 0;
                end_index = 7;
                can_scroll_down = true;
            } else if (selected_index >= option_count - 4) {
                start_index = option_count - 7;
                end_index = option_count;
                can_scroll_up = true;
            } else {
                start_index = selected_index - 3;
                end_index = selected_index + 4;
                can_scroll_up = true;
                can_scroll_down = true;
            }
        }
        for (size_t i = start_index; i < end_index; ++i) {
            if (i == selected_index) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.println(options[i]);
        }
        if (can_scroll_up || can_scroll_down) {
            // Draw a scrollbar
            constexpr size_t padding = 1;
            size_t max_scrollbar_height = 64 - 8 - padding * 2 - 2; // 8 pixels for titlebar + padding
            display.drawRect(SCREEN_WIDTH - 4, 8 + padding, 4, max_scrollbar_height + 2, SSD1306_WHITE);
            uint16_t scrollbar_height = max_scrollbar_height * 7 / option_count;
            size_t max_offset = max_scrollbar_height - static_cast<size_t>(scrollbar_height);
            uint16_t scrollbar_offset = max_offset * selected_index / (option_count - 1);
            display.fillRect(SCREEN_WIDTH - 4, 8 + 1 + padding + scrollbar_offset, 4, scrollbar_height, SSD1306_WHITE);
        }
        display.display();
    }

    void generic_cursor_up(size_t &cursor, size_t option_count) {
        if (cursor == 0) {
            cursor = option_count - 1;
        } else {
            cursor--;
        }
    }

    void generic_cursor_down(size_t &cursor, size_t option_count) {
        cursor = (cursor + 1) % option_count;
    }

    void handle_generic_menu_navigation(
        events::Event ev, 
        size_t option_count, 
        size_t &cursor, 
        Adafruit_SSD1306& display,
        void(*action)(size_t cursor, Adafruit_SSD1306& display),
        void(*back_action)(Adafruit_SSD1306& display),
        bool play_confirm_tone, 
        bool play_cancel_tone, 
        bool play_navigation_tone
    ) {
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::UP:
                        if (play_navigation_tone) {
                            sound::play_navigation_tone();
                        }
                        generic_cursor_up(cursor, option_count);
                        set_dirty();
                        break;
                    case events::Button::DOWN:
                        if (play_navigation_tone) {
                            sound::play_navigation_tone();
                        }
                        generic_cursor_down(cursor, option_count);
                        set_dirty();
                        break;
                    case events::Button::A:
                        if (play_confirm_tone) {
                            sound::play_confirm_tone();
                        }
                        action(cursor, display);
                        set_dirty();
                        break;
                    case events::Button::B:
                        if (back_action != nullptr) {
                            if (play_cancel_tone) {
                                sound::play_cancel_tone();
                            }
                            back_action(display);
                            set_dirty();
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    void draw_main_menu(Adafruit_SSD1306 &display) {
        draw_generic_menu(display, "Main Menu", menu_items, sizeof(menu_items)/sizeof(menu_items[0]), cursor);
    }

    void status_update_task(void* param) {
        while (true) {
            wifi::WiFiStatus current_wifi_status = wifi::get_status();
            battery::BatteryStatus current_battery_status = battery::get_battery_status();
            bool alarm_is_set = apps::alarm::get_alarm_timestamp().timestamp != 0;
            if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
                if (current_wifi_status != last_wifi_status) {
                    last_wifi_status = current_wifi_status;
                    dirty = true;
                }
                if (current_battery_status.level != last_battery_level) {
                    last_battery_level = current_battery_status.level;
                    dirty = true;
                }
                if (alarm_is_set != last_alarm_set) {
                    last_alarm_set = alarm_is_set;
                    dirty = true;
                }
                xSemaphoreGive(status_mutex);
            }
            vTaskDelay(pdMS_TO_TICKS(UPDATE_STATUS_INTERVAL_MS)); // Update every 1 seconds
        }
    }

    void init() {
        status_mutex = xSemaphoreCreateMutex();
        xTaskCreate(status_update_task, "StatusUpdate", 2048, nullptr, 1, nullptr);
    }

    void upkeep(Adafruit_SSD1306& display, uint64_t delay_ms) {
        bool needs_redraw = false;
        if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
            needs_redraw = dirty;
            dirty = false;
            xSemaphoreGive(status_mutex);
        }
        if (needs_redraw) {
            switch (current_app) {
                case App::NONE:
                    draw_main_menu(display);
                    break;
                default:
                    if (app_draws[static_cast<size_t>(current_app) - 1] != nullptr) {
                        app_draws[static_cast<size_t>(current_app) - 1](display);
                    }
                    break;
            }
        }
        auto last_event_timestamp = events::get_last_event_timestamp();
        if (last_event_timestamp + TIME_BEFORE_DEEPSLEEP_US < timekeeper::now_us()) {
            deepsleep::deepsleep(display);
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms)); // Small delay to prevent busy looping
    }

    void cursor_up() {
        if (cursor == 0) {
            cursor = sizeof(menu_items)/sizeof(menu_items[0]) - 1;
        } else {
            cursor--;
        }
    }

    void cursor_down() {
        cursor = (cursor + 1) % (sizeof(menu_items)/sizeof(menu_items[0]));
    }

    void main_menu_actions(size_t cursor, Adafruit_SSD1306& display) {
        current_app = static_cast<App>(cursor + 1);
    }

    void main_menu(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        handle_generic_menu_navigation(ev, sizeof(menu_items)/sizeof(menu_items[0]), cursor, display, main_menu_actions);
        switch (ev.type) {
            case events::EventType::NONE:
                upkeep(display);
                break;
            default:
                break;
        }
    }

    void set_dirty()
    {
        if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
            dirty = true;
            xSemaphoreGive(status_mutex);
        }
    }

    void main_loop(Adafruit_SSD1306 &display)
    {
        switch (current_app) {
            case App::NONE:
                main_menu(display);
                break;
            default:
                if (app_mains[static_cast<size_t>(current_app) - 1] != nullptr) {
                    app_mains[static_cast<size_t>(current_app) - 1](display);
                }
                break;
        }
    }

    enum class KBKey {
        S_backtick, S_tilde, S_exclam, S_at, S_hash, S_dollar, S_percent, S_caret, S_ampersand, S_asterisk, 
        S_left_paren, S_right_paren, S_minus, S_underscore, S_equal, S_plus, S_left_brace, S_right_brace,
        S_left_bracket, S_right_bracket, S_backslash, S_pipe, S_semicolon, S_colon, S_quote, S_double_quote,
        S_comma, S_less_than, S_period, S_greater_than, S_slash, S_question,
        N_1, N_2, N_3, N_4, N_5, N_6, N_7, N_8, N_9, N_0,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
        TAB, CAPS, SPACE, BACKSPACE, ENTER, SHIFT, NONE
    };

    const char* kbkey_to_string(KBKey key) {
        switch (key) {
            case KBKey::S_backtick: return "`";
            case KBKey::S_tilde: return "~";
            case KBKey::S_exclam: return "!";
            case KBKey::S_at: return "@";
            case KBKey::S_hash: return "#";
            case KBKey::S_dollar: return "$";
            case KBKey::S_percent: return "%";
            case KBKey::S_caret: return "^";
            case KBKey::S_ampersand: return "&";
            case KBKey::S_asterisk: return "*";
            case KBKey::S_left_paren: return "(";
            case KBKey::S_right_paren: return ")";
            case KBKey::S_minus: return "-";
            case KBKey::S_underscore: return "_";
            case KBKey::S_equal: return "=";
            case KBKey::S_plus: return "+";
            case KBKey::S_left_brace: return "{";
            case KBKey::S_right_brace: return "}";
            case KBKey::S_left_bracket: return "[";
            case KBKey::S_right_bracket: return "]";
            case KBKey::S_backslash: return "\\";
            case KBKey::S_pipe: return "|";
            case KBKey::S_semicolon: return ";";
            case KBKey::S_colon: return ":";
            case KBKey::S_quote: return "'";
            case KBKey::S_double_quote: return "\"";
            case KBKey::S_comma: return ",";
            case KBKey::S_less_than: return "<";
            case KBKey::S_period: return ".";
            case KBKey::S_greater_than: return ">";
            case KBKey::S_slash: return "/";
            case KBKey::S_question: return "?";
            case KBKey::N_1: return "1";
            case KBKey::N_2: return "2";
            case KBKey::N_3: return "3";
            case KBKey::N_4: return "4";
            case KBKey::N_5: return "5";
            case KBKey::N_6: return "6";
            case KBKey::N_7: return "7";
            case KBKey::N_8: return "8";
            case KBKey::N_9: return "9";
            case KBKey::N_0: return "0";
            case KBKey::A: return "A";
            case KBKey::B: return "B";
            case KBKey::C: return "C";
            case KBKey::D: return "D";
            case KBKey::E: return "E";
            case KBKey::F: return "F";
            case KBKey::G: return "G";
            case KBKey::H: return "H";
            case KBKey::I: return "I";
            case KBKey::J: return "J";
            case KBKey::K: return "K";
            case KBKey::L: return "L";
            case KBKey::M: return "M";
            case KBKey::N: return "N";
            case KBKey::O: return "O";
            case KBKey::P: return "P";
            case KBKey::Q: return "Q";
            case KBKey::R: return "R";
            case KBKey::S: return "S";
            case KBKey::T: return "T";
            case KBKey::U: return "U";
            case KBKey::V: return "V";
            case KBKey::W: return "W";
            case KBKey::X: return "X";
            case KBKey::Y: return "Y";
            case KBKey::Z: return "Z";
            case KBKey::a: return "a";
            case KBKey::b: return "b";
            case KBKey::c: return "c";
            case KBKey::d: return "d";
            case KBKey::e: return "e";
            case KBKey::f: return "f";
            case KBKey::g: return "g";
            case KBKey::h: return "h";
            case KBKey::i: return "i";
            case KBKey::j: return "j";
            case KBKey::k: return "k";
            case KBKey::l: return "l";
            case KBKey::m: return "m";
            case KBKey::n: return "n";
            case KBKey::o: return "o";
            case KBKey::p: return "p";
            case KBKey::q: return "q";
            case KBKey::r: return "r";
            case KBKey::s: return "s";
            case KBKey::t: return "t";
            case KBKey::u: return "u";
            case KBKey::v: return "v";
            case KBKey::w: return "w";
            case KBKey::x: return "x";
            case KBKey::y: return "y";
            case KBKey::z: return "z";
            case KBKey::TAB: return " TAB ";
            case KBKey::CAPS: return " CAPS ";
            case KBKey::SPACE: return " SPACE ";
            case KBKey::BACKSPACE: return " BS ";
            case KBKey::ENTER: return " ENTER ";
            case KBKey::SHIFT: return " SHIFT ";
            default: return "";
        }
    }
    
    template<uint16_t Keys, uint8_t Rows>
    struct KBLayout {
        const KBKey normal[Keys];
        const KBKey shifted[Keys];
        const uint16_t cols_per_row[Rows];
    };

    const KBLayout<61, 6> keyboard_layout = {
        .normal = {
            KBKey::S_backtick, KBKey::N_1, KBKey::N_2, KBKey::N_3, KBKey::N_4, KBKey::N_5, KBKey::N_6, KBKey::N_7, KBKey::N_8, KBKey::N_9, KBKey::N_0, KBKey::S_minus, KBKey::S_equal,
            KBKey::q, KBKey::w, KBKey::e, KBKey::r, KBKey::t, KBKey::y, KBKey::u, KBKey::i, KBKey::o, KBKey::p, KBKey::S_left_bracket, KBKey::S_right_bracket, KBKey::S_backslash,
            KBKey::a, KBKey::s, KBKey::d, KBKey::f, KBKey::g, KBKey::h, KBKey::j, KBKey::k, KBKey::l, KBKey::S_semicolon, KBKey::S_quote,
            KBKey::z, KBKey::x, KBKey::c, KBKey::v, KBKey::b, KBKey::n, KBKey::m, KBKey::S_comma, KBKey::S_period, KBKey::S_slash,
            KBKey::SHIFT, KBKey::CAPS, KBKey::TAB,
            KBKey::BACKSPACE, KBKey::SPACE, KBKey::ENTER
        },
        .shifted = {
            KBKey::S_tilde, KBKey::S_exclam, KBKey::S_at, KBKey::S_hash, KBKey::S_dollar, KBKey::S_percent, KBKey::S_caret, KBKey::S_ampersand, KBKey::S_asterisk, KBKey::S_left_paren, KBKey::S_right_paren, KBKey::S_underscore, KBKey::S_plus,
            KBKey::Q, KBKey::W, KBKey::E, KBKey::R, KBKey::T, KBKey::Y, KBKey::U, KBKey::I, KBKey::O, KBKey::P, KBKey::S_left_brace, KBKey::S_right_brace, KBKey::S_pipe,
            KBKey::A, KBKey::S, KBKey::D, KBKey::F, KBKey::G, KBKey::H, KBKey::J, KBKey::K, KBKey::L, KBKey::S_colon, KBKey::S_double_quote,
            KBKey::Z, KBKey::X, KBKey::C, KBKey::V, KBKey::B, KBKey::N, KBKey::M, KBKey::S_less_than, KBKey::S_greater_than, KBKey::S_question,
            KBKey::SHIFT, KBKey::CAPS, KBKey::TAB,
            KBKey::BACKSPACE, KBKey::SPACE, KBKey::ENTER
        },
        .cols_per_row = {
            13, 13, 11, 10, 3, 3
        }
    };

    uint8_t get_kb_row(size_t selected_key) {
        size_t key_index = 0;
        for (size_t row = 0; row < sizeof(keyboard_layout.cols_per_row)/sizeof(keyboard_layout.cols_per_row[0]); ++row) {
            size_t cols = keyboard_layout.cols_per_row[row];
            if (selected_key < key_index + cols) {
                return row;
            }
            key_index += cols;
        }
        return 0;
    }

    void get_kb_min_max_index_for_row(uint8_t row, size_t & min_index, size_t & max_index) {
        size_t key_index = 0;
        for (size_t r = 0; r < sizeof(keyboard_layout.cols_per_row)/sizeof(keyboard_layout.cols_per_row[0]); ++r) {
            size_t cols = keyboard_layout.cols_per_row[r];
            if (r == row) {
                min_index = key_index;
                max_index = key_index + cols - 1;
                return;
            }
            key_index += cols;
        }
        min_index = 0;
        max_index = 0;
    }

    void kb_move_left(KBStatus & kb_status) {
        auto current_row = get_kb_row(kb_status.selected_key);
        size_t min_index, max_index;
        get_kb_min_max_index_for_row(current_row, min_index, max_index);
        if (kb_status.selected_key == min_index) {
            kb_status.selected_key = max_index;
        } else {
            kb_status.selected_key--;
        }
    }

    void kb_move_right(KBStatus & kb_status) {
        auto current_row = get_kb_row(kb_status.selected_key);
        size_t min_index, max_index;
        get_kb_min_max_index_for_row(current_row, min_index, max_index);
        if (kb_status.selected_key == max_index) {
            kb_status.selected_key = min_index;
        } else {
            kb_status.selected_key++;
        }
    }

    void kb_move_up(KBStatus & kb_status) {
        auto current_row = get_kb_row(kb_status.selected_key);
        uint8_t target_row;
        if (current_row == 0) {
            target_row = sizeof(keyboard_layout.cols_per_row)/sizeof(keyboard_layout.cols_per_row[0]) - 1;
        } else {
            target_row = current_row - 1;
        }
        // Lerp to the closest key in the target row
        size_t current_row_min, current_row_max;
        get_kb_min_max_index_for_row(current_row, current_row_min, current_row_max);
        size_t target_row_min, target_row_max;
        get_kb_min_max_index_for_row(target_row, target_row_min, target_row_max);
        size_t current_row_cols = current_row_max - current_row_min + 1;
        size_t target_row_cols = target_row_max - target_row_min + 1;
        size_t position_in_row = kb_status.selected_key - current_row_min;
        size_t target_position = position_in_row * target_row_cols / current_row_cols;
        kb_status.selected_key = target_row_min + target_position;
    }

    void kb_move_down(KBStatus & kb_status) {
        auto current_row = get_kb_row(kb_status.selected_key);
        uint8_t target_row;
        if (current_row == sizeof(keyboard_layout.cols_per_row)/sizeof(keyboard_layout.cols_per_row[0]) - 1) {
            target_row = 0;
        } else {
            target_row = current_row + 1;
        }
        // Lerp to the closest key in the target row
        size_t current_row_min, current_row_max;
        get_kb_min_max_index_for_row(current_row, current_row_min, current_row_max);
        size_t target_row_min, target_row_max;
        get_kb_min_max_index_for_row(target_row, target_row_min, target_row_max);
        size_t current_row_cols = current_row_max - current_row_min + 1;
        size_t target_row_cols = target_row_max - target_row_min + 1;
        size_t position_in_row = kb_status.selected_key - current_row_min;
        size_t target_position = position_in_row * target_row_cols / current_row_cols;
        kb_status.selected_key = target_row_min + target_position;
    }

    KBEvent kb_handle_character_selection(
        KBStatus & kb_status, 
        String & input_buffer
    ) {
        KBEvent event = KBEvent::NONE;
        KBKey selected_key = kb_status.caps_active || kb_status.shift_active ? keyboard_layout.shifted[kb_status.selected_key] : keyboard_layout.normal[kb_status.selected_key];
        switch (selected_key) {
            case KBKey::SHIFT:
                if (kb_status.caps_active) {
                    kb_status.caps_active = false;
                    kb_status.shift_active = false;
                } else {
                    kb_status.shift_active = !kb_status.shift_active;
                }
                break;
            case KBKey::CAPS:
                if (kb_status.shift_active) {
                    kb_status.shift_active = false;
                }
                kb_status.caps_active = !kb_status.caps_active;
                break;
            case KBKey::BACKSPACE:
                if (input_buffer.length() > 0) {
                    input_buffer.remove(input_buffer.length() - 1);
                    event = KBEvent::DELETED_CHARACTER;
                }
                break;
            case KBKey::TAB:
                event = KBEvent::TAB_PRESSED;
                break;
            case KBKey::SPACE:
                input_buffer += ' ';
                event = KBEvent::ENTERED_CHARACTER;
                break;
            case KBKey::ENTER:
                event = KBEvent::ENTER_PRESSED;
                break;
            default:
                input_buffer += kbkey_to_string(selected_key);
                event = KBEvent::ENTERED_CHARACTER;
                if (kb_status.shift_active) {
                    kb_status.shift_active = false;
                }
                break;
        }
        return event;
    }

    KBEvent handle_keyboard_input(
        events::Event ev, 
        KBStatus & kb_status, 
        String & input_buffer
    ) {
        KBEvent event = KBEvent::NONE;
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::LEFT:
                        sound::play_navigation_tone();
                        kb_move_left(kb_status);
                        set_dirty();
                        break;
                    case events::Button::RIGHT:
                        sound::play_navigation_tone();
                        kb_move_right(kb_status);
                        set_dirty();
                        break;
                    case events::Button::UP:
                        sound::play_navigation_tone();
                        kb_move_up(kb_status);
                        set_dirty();
                        break;
                    case events::Button::DOWN:
                        sound::play_navigation_tone();
                        kb_move_down(kb_status);
                        set_dirty();
                        break;
                    case events::Button::A:
                        sound::play_confirm_tone();
                        event = kb_handle_character_selection(kb_status, input_buffer);
                        set_dirty();
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        event = KBEvent::KEYBOARD_CLOSED;
                        set_dirty();
                        break;
                    default:
                        break;
                }
        }
        return event;
    }

    void draw_keyboard(
        Adafruit_SSD1306& display, 
        const KBStatus& kb_status, 
        const String& input_buffer
    ) {
        auto shift_active = kb_status.caps_active || kb_status.shift_active;
        auto selected_key = kb_status.selected_key;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_INVERSE);
        display.setCursor(0, 0);
        auto max_shown_chars = SCREEN_WIDTH / 6 - 1;
        auto padding = (SCREEN_WIDTH - (max_shown_chars + 1) * 6) / 2; // Center the text
        size_t start_pos = 0;
        if (input_buffer.length() > max_shown_chars) {
            start_pos = input_buffer.length() - max_shown_chars;
        }
        auto shown_chars = MIN(input_buffer.length(), max_shown_chars);
        display.setCursor(padding, 0);
        display.print(input_buffer.substring(start_pos, start_pos + max_shown_chars));
        display.drawFastHLine(padding + shown_chars * 6, 9, 6, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.drawFastHLine(0, 11, SCREEN_WIDTH, SSD1306_WHITE);
        size_t key_index = 0;
        size_t y_offset = 13;
        const size_t char_height = 8;
        for (size_t row = 0; row < sizeof(keyboard_layout.cols_per_row)/sizeof(keyboard_layout.cols_per_row[0]); ++row) {
            size_t cols = keyboard_layout.cols_per_row[row];
            size_t x_offset = 0;
            size_t special_keys = 0;
            size_t special_keys_width = 0;
            size_t key_index_backup = key_index;
            for (size_t col = 0; col < cols; ++col) {
                KBKey key = shift_active ? keyboard_layout.shifted[key_index] : keyboard_layout.normal[key_index];
                auto str = kbkey_to_string(key);
                auto len = strlen(str);
                if (len > 1) {
                    special_keys++;
                    special_keys_width += 6 * len;
                }
                key_index++;
            }
            key_index = key_index_backup;
            auto size_per_key = (SCREEN_WIDTH - special_keys_width) / (cols - special_keys);
            auto extra_space = (SCREEN_WIDTH - special_keys_width) % (cols - special_keys);
            uint8_t extra_space_used = 0;
            for (size_t col = 0; col < cols; ++col) {
                KBKey key = shift_active ? keyboard_layout.shifted[key_index] : keyboard_layout.normal[key_index];
                auto str = kbkey_to_string(key);
                auto chars = strlen(str);
                size_t width = (chars > 1) ? (6 * chars) : size_per_key;
                if (chars == 1 && extra_space_used < extra_space) {
                    width += 1;
                    extra_space_used++;
                }
                display.setTextColor(SSD1306_INVERSE);
                if (key_index == selected_key) {
                    display.fillRect(x_offset, y_offset, width, char_height, SSD1306_WHITE);
                }
                auto char_start_x = x_offset + (width - (6 * chars)) / 2;
                display.setCursor(char_start_x, y_offset);
                display.print(str);
                x_offset += width;
                key_index++;
            }
            y_offset += char_height;
        }
        display.display();
    }

    BooleanSwitchEvent handle_boolean_switch_input(
        events::Event ev, 
        bool & value, 
        Adafruit_SSD1306& display
    ) {
        BooleanSwitchEvent event = BooleanSwitchEvent::NONE;
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::LEFT:
                        if (!value) {
                            sound::play_navigation_tone();
                            value = true;
                            event = BooleanSwitchEvent::TOGGLED;
                            set_dirty();
                        }
                        break;
                    case events::Button::RIGHT:
                        if (value) {
                            sound::play_navigation_tone();
                            value = false;
                            event = BooleanSwitchEvent::TOGGLED;
                            set_dirty();
                        }
                        break;
                    case events::Button::A:
                        sound::play_confirm_tone();
                        value = !value;
                        event = BooleanSwitchEvent::TOGGLED;
                        set_dirty();
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        event = BooleanSwitchEvent::CLOSED;
                        set_dirty();
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        return event;
    }

    void draw_boolean_switch(Adafruit_SSD1306& display, const char* title, bool value) {
        display.clearDisplay();
        draw_generic_titlebar(display, title);
        display.setTextColor(SSD1306_INVERSE);
        display.drawRoundRect(30, 25, 60, 30, 16, SSD1306_WHITE);
        if (value) {
            display.fillRoundRect(32, 27, 56, 26, 14, SSD1306_WHITE);
            display.drawRoundRect(31, 26, 28, 28, 14, SSD1306_BLACK);
            display.setCursor(39, 37);
            display.print("ON");
        } else {
            display.fillRoundRect(62, 27, 26, 26, 14, SSD1306_WHITE);
            display.setCursor(68, 37);
            display.print("OFF");
        }
        display.display();
    }

    ConfirmationDialogResult handle_confirmation_dialog_input(
        events::Event ev, 
        Adafruit_SSD1306& display
    ) {
        ConfirmationDialogResult result = ConfirmationDialogResult::NONE;
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::A:
                        sound::play_confirm_tone();
                        result = ConfirmationDialogResult::CONFIRMED;
                        set_dirty();
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        result = ConfirmationDialogResult::CANCELED;
                        set_dirty();
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        return result;
    }

    void draw_confirmation_dialog(
        Adafruit_SSD1306& display, 
        const char* message
    ) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(message);
        display.drawFastHLine(0, 53, SCREEN_WIDTH, SSD1306_WHITE);
        display.setCursor(16, 55);
        display.print("A: Yes   B: No");
        display.display();
    }
}