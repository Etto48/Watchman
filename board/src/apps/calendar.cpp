#include "apps/calendar.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace apps::calendar {

    const char* month_names[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    const char* short_day_names[] = {
        "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"
    };

    const char* full_day_names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    tm selected_date = {0};
    tm today = {0};
    uint64_t last_today_update_us = 0;
    constexpr uint64_t today_update_interval_us = 60000000; // Update "today" every minute
    bool initialized = false;

    bool day_selected = false;

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::UP:
                        sound::play_navigation_tone();
                        selected_date.tm_mday -= 7;
                        mktime(&selected_date); // Normalize date
                        menu::set_dirty();
                        break;
                    case events::Button::DOWN:
                        sound::play_navigation_tone();
                        selected_date.tm_mday += 7;
                        mktime(&selected_date); // Normalize date
                        menu::set_dirty();
                        break;
                    case events::Button::LEFT:
                        sound::play_navigation_tone();
                        selected_date.tm_mday -= 1;
                        mktime(&selected_date); // Normalize date
                        menu::set_dirty();
                        break;
                    case events::Button::RIGHT:
                        sound::play_navigation_tone();
                        selected_date.tm_mday += 1;
                        mktime(&selected_date); // Normalize date
                        menu::set_dirty();
                        break;
                    case events::Button::A:
                        sound::play_confirm_tone();
                        day_selected = !day_selected;
                        menu::set_dirty();
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        if (day_selected) {
                            day_selected = false;
                        } else {
                            initialized = false;
                            menu::current_app = menu::App::NONE;
                        }
                        menu::set_dirty();
                        break;
                }       
                break;
            case events::EventType::NONE:
                if (!initialized) {
                    getLocalTime(&selected_date, 0);
                    initialized = true;
                }
                if (last_today_update_us + today_update_interval_us <= timekeeper::now_us() || last_today_update_us == 0) {
                    tm temp_today = {0};
                    getLocalTime(&temp_today, 0);
                    if (temp_today.tm_mday != today.tm_mday ||
                        temp_today.tm_mon != today.tm_mon ||
                        temp_today.tm_year != today.tm_year) {
                        today = temp_today;
                        menu::set_dirty();
                    }
                    last_today_update_us = timekeeper::now_us();
                }
                menu::upkeep(display);
                break;
        }
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        if (day_selected) {
            display.setTextSize(1);
            display.setTextColor(SSD1306_INVERSE);
            display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_INVERSE);
            display.setCursor(10, 0);
            display.printf(
                "%s - %04d",
                month_names[selected_date.tm_mon],
                selected_date.tm_year + 1900
            );
            display.setCursor(10, 10);
            display.printf(
                "%s %02d",
                full_day_names[selected_date.tm_wday],
                selected_date.tm_mday
            );
            display.drawFastHLine(0, 19, SCREEN_WIDTH, SSD1306_WHITE);
            display.setCursor(10, 20);
            display.printf(
                "%d days from today",
                (mktime(&selected_date) - mktime(&today)) / 86400
            );
            display.setTextSize(1);
            display.setCursor(10, 30);
        } else {
            display.setTextSize(1);
            display.setTextColor(SSD1306_INVERSE);
            display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_INVERSE);
            display.setCursor(10, 0);
            display.printf("%s - %04d\n", month_names[selected_date.tm_mon], selected_date.tm_year + 1900);
            // Draw day names
            for (int i = 0; i < 7; i++) {
                display.setCursor(2 + i * 18, 8);
                display.printf("%s", short_day_names[i]);
            }
            tm cursor = selected_date;
            cursor.tm_mday = 1;
            mktime(&cursor); // Normalize to get correct tm_wday
            auto row = 0;
            while (cursor.tm_mon == selected_date.tm_mon) {
                auto x_offset = 2 + (cursor.tm_wday % 7) * 18;
                auto y_offset = 16 + row * 8;
                if (cursor.tm_mday == selected_date.tm_mday) {
                    display.fillRect(x_offset - 1, y_offset - 1, 16, 9, SSD1306_WHITE);
                } else if (cursor.tm_mday == today.tm_mday &&
                        cursor.tm_mon == today.tm_mon &&
                        cursor.tm_year == today.tm_year) {
                    display.drawRect(x_offset - 1, y_offset - 1, 16, 9, SSD1306_WHITE);
                }
                display.setCursor(x_offset, y_offset);
                display.printf("%2d", cursor.tm_mday);
                if (cursor.tm_wday == 6) {
                    row++;
                }
                cursor.tm_mday++;
                mktime(&cursor); // Normalize to get correct tm_wday
            }
        }
        display.display();
    }
}