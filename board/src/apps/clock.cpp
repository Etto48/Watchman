#include "apps/clock.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace apps::clock {
    tm timeinfo = {0};
    uint64_t last_time_info_update_us = 0;

    enum class ClockMode {
        SINCE_BOOT,
        REAL_TIME
    };
    ClockMode clock_mode = ClockMode::SINCE_BOOT;

    bool valid_timeinfo(tm& t) {
        return (t.tm_year + 1900 >= 2025); // Consider time valid if year is 2025 or later
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Clock");
        uint16_t selector_offset;
        if (clock_mode == ClockMode::SINCE_BOOT) {
            selector_offset = 0;
        } else {
            selector_offset = SCREEN_WIDTH / 2;
        }
        display.fillRect(selector_offset, 8, SCREEN_WIDTH/2, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_INVERSE);
        display.setTextSize(1);
        display.setCursor(16, 8);
        display.println("BOOT");
        display.setCursor(SCREEN_WIDTH - 16 - 3*6, 8);
        display.println("RTC");
        display.setTextSize(2);
        display.setCursor(10, 20);
        if (clock_mode == ClockMode::SINCE_BOOT) { // SINCE_BOOT
            uint64_t micros = timekeeper::now_us();
            auto total_seconds = micros / 1000000;
            auto days = total_seconds / 86400;
            auto hours = (total_seconds % 86400) / 3600;
            auto minutes = (total_seconds % 3600) / 60;
            auto seconds = total_seconds % 60;
            char buffer[30];
            display.setTextSize(1);
            display.setCursor(10, 18);
            snprintf(buffer, sizeof(buffer), "%lld day%s",
                     days, days == 1 ? "" : "s");
            display.println(buffer);
            display.setCursor(10, 32);
            display.setTextSize(2);
            snprintf(buffer, sizeof(buffer), "%02lld:%02lld:%02lld",
                    hours, minutes, seconds);
            display.println(buffer);
            display.setTextSize(1);
            display.setCursor(10, 50);
            display.println("since boot");
        } else if (!valid_timeinfo(timeinfo)) { // REAL_TIME but not valid
            display.println("No RTC");
            display.setTextSize(1);
            display.setCursor(10, 40);
            display.println("Connect WiFi");
            display.setCursor(10, 48);
            display.println("for NTP sync");
        } else { // REAL_TIME and valid
            char buffer[20];
            strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
            display.println(buffer);
            display.setTextSize(1);
            display.setCursor(10, 50);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
            display.println(buffer);
        }
        display.display();
    }

    void app(Adafruit_SSD1306& display) {
        constexpr uint64_t time_info_update_interval_us = 1000000; // Update time info every second
        // Implementation for the time app logic
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_event.button) {
                    case events::Button::A:
                    case events::Button::RIGHT:
                    case events::Button::LEFT:
                        sound::play_navigation_tone();
                        if (clock_mode == ClockMode::REAL_TIME) {
                            clock_mode = ClockMode::SINCE_BOOT;
                        } else {
                            clock_mode = ClockMode::REAL_TIME;
                        }
                        menu::set_dirty();
                        break;
                    case events::Button::B:
                        menu::current_app = menu::App::NONE;
                        sound::play_cancel_tone();
                        menu::set_dirty();
                        break;
                    default:
                        break;
                }    
                break;
            case events::EventType::NONE:
                if (last_time_info_update_us == 0) {
                    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
                }
                if (last_time_info_update_us + time_info_update_interval_us <= timekeeper::now_us()) {
                    // Update timeinfo here
                    last_time_info_update_us = timekeeper::now_us();
                    getLocalTime(&timeinfo, 0);
                    if (valid_timeinfo(timeinfo) || clock_mode == ClockMode::SINCE_BOOT) {
                        // Mark display as dirty to trigger redraw
                        menu::set_dirty();
                    }
                }
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }
}