#include "apps/stopwatch.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"

namespace apps::stopwatch {
    uint64_t start_time_us = 0;
    uint64_t pause_time_us = 0;

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Stopwatch");
        uint64_t elapsed_us;
        if (start_time_us == 0) {
            elapsed_us = 0;
        } else if (pause_time_us == 0) {
            elapsed_us = timekeeper::now_us() - start_time_us;
        } else {
            elapsed_us = pause_time_us - start_time_us;
        }
        uint64_t ms = elapsed_us / 1000 % 1000;
        uint64_t seconds = (elapsed_us / 1000000) % 60;
        uint64_t minutes = (elapsed_us / 60000000);

        display.setTextSize(2);
        display.setCursor(26, 20);
        display.printf("%02llu:%02llu", minutes, seconds);
        display.setTextSize(1);
        display.setCursor(86, 26);
        display.printf(".%03llu", ms);
        display.setCursor(16, 50);
        if (start_time_us == 0) {
            display.println("Press A to Start");
        } else if (pause_time_us == 0) {
            display.println("Press A to Pause");
        } else {
            display.println("Press A to Reset");
        }
        display.display();
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.buttonEvent.button) {
                    case events::Button::A:
                        sound::play_confirm_tone();
                        if (start_time_us == 0) {
                            start_time_us = timekeeper::now_us();
                            pause_time_us = 0;
                        } else if (pause_time_us == 0) {
                            pause_time_us = timekeeper::now_us();
                        } else {
                            start_time_us = 0;
                            pause_time_us = 0;
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
            case events::EventType::NONE:
                if (pause_time_us == 0 && start_time_us != 0) {
                    menu::set_dirty(); // Continuously update the display while running
                    menu::upkeep(display, 0);
                } else {
                    menu::upkeep(display);
                }
                break;
            default:
                break;      
        }
    }
}