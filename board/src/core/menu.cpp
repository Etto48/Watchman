#include <cstdint>

#include "core/menu.hpp"
#include "core/events.hpp"
#include "core/sound.hpp"
#include "core/image.hpp"
#include "apps/temperature.hpp"
#include "apps/music.hpp"
#include "apps/weather.hpp"
#include "apps/deepsleep.hpp"
#include "apps/stopwatch.hpp"
#include "apps/timer.hpp"
#include "core/deepsleep.hpp"
#include "constants.hpp"
#include "images/wifi_0.hpp"
#include "images/wifi_1.hpp"
#include "images/wifi_2.hpp"
#include "images/wifi_3.hpp"
#include "images/battery_0.hpp"
#include "images/battery_1.hpp"
#include "images/battery_2.hpp"
#include "images/battery_3.hpp"
#include "apps/clock.hpp"
#include "core/wifi.hpp"
#include "core/battery.hpp"
#include "core/menu.hpp"
#include "core/timekeeper.hpp"

namespace menu {
    static SemaphoreHandle_t status_mutex = nullptr;
    bool dirty = true;
    wifi::WiFiStatus last_wifi_status = wifi::WiFiStatus::DISCONNECTED;
    battery::BatteryLevel last_battery_level = battery::BatteryLevel::BATTERY_EMPTY;
    
    App current_app = App::NONE;
    size_t cursor = 0;

    constexpr const char *menu_items[] = {
        "Temperature",
        "Clock",
        "Stopwatch",
        "Timer",
        "Music",
        "Weather",
        "Deepsleep"
    };
    constexpr void(*app_mains[])(Adafruit_SSD1306&) = {
        apps::temperature::app,
        apps::clock::app,
        apps::stopwatch::app,
        apps::timer::app,
        apps::music::app,
        apps::weather::app,
        apps::deepsleep::app
    };
    constexpr void(*app_draws[])(Adafruit_SSD1306&) = {
        apps::temperature::draw,
        apps::clock::draw,
        apps::stopwatch::draw,
        apps::timer::draw,
        apps::music::draw,
        apps::weather::draw,
        apps::deepsleep::draw
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
            default:
                battery_icon = images::battery_0;
                break;
        }
        display.drawBitmap(SCREEN_WIDTH - images::battery_0_width * 2, 0, battery_icon, images::battery_0_width, images::battery_0_height, SSD1306_BLACK);
    }

    void draw_generic_titlebar(Adafruit_SSD1306 &display, const char *title)
    {
        display.setTextSize(1);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(0, 0);
        display.print(" ");
        display.println(title);
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
        bool play_confirm_tone, 
        bool play_cancel_tone, 
        bool play_navigation_tone
    ) {
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.buttonEvent.button) {
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
                        sound::play_cancel_tone();
                        menu::current_app = menu::App::NONE;
                        set_dirty();
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
            battery::BatteryLevel current_battery_level = battery::get_battery_level();
            if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
                if (current_wifi_status != last_wifi_status) {
                    last_wifi_status = current_wifi_status;
                    dirty = true;
                }
                if (current_battery_level != last_battery_level) {
                    last_battery_level = current_battery_level;
                    dirty = true;
                }
                xSemaphoreGive(status_mutex);
            }
            vTaskDelay(pdMS_TO_TICKS(UPDATE_STATUS_INTERVAL_MS)); // Update every 1 seconds
        }
    }

    void init() {
        configTime(GMT_OFFSET_S, DAYLIGHT_OFFSET_S, "pool.ntp.org", "time.nist.gov");
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
}