#include "apps/time.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"

namespace apps::time {
    tm timeinfo = {0};
    uint64_t last_time_info_update = 0;

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        display.setTextSize(1);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(0, 0);
        display.println(" Time");
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(10, 20);
        if (timeinfo.tm_year + 1900 < 2025) {
            int64_t micros = esp_timer_get_time();
            auto total_seconds = micros / 1000000;
            auto days = total_seconds / 86400;
            auto hours = (total_seconds % 86400) / 3600;
            auto minutes = (total_seconds % 3600) / 60;
            auto seconds = total_seconds % 60;
            char buffer[30];
            display.setTextSize(1);
            display.setCursor(10, 16);
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
        } else {
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
        constexpr uint64_t time_info_update_interval_ms = 1000; // Update time info every second
        // Implementation for the time app logic
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.buttonEvent.button) {
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
                if (last_time_info_update == 0) {
                    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
                }
                if (last_time_info_update + time_info_update_interval_ms <= millis()) {
                    // Update timeinfo here
                    last_time_info_update = millis();
                    getLocalTime(&timeinfo, 0);
                    // Mark display as dirty to trigger redraw
                    menu::set_dirty();
                }
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }
}