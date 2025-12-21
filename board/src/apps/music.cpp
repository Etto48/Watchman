#include "apps/music.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"
#include "melodies/cmajor.hpp"
#include "melodies/twinkle.hpp"
#include "melodies/joy.hpp"

namespace apps::music {
    uint64_t cursor = 0;

    constexpr const char* melodies[] = {
        "C Major Scale",
        "Twinkle",
        "Ode to Joy"
    };

    void play_melody() {
        switch (cursor) {
            case 0:
                sound::async_play_melody(melodies::cmajor, sizeof(melodies::cmajor)/sizeof(melodies::cmajor[0]));
                break;
            case 1:
                sound::async_play_melody(melodies::twinkle, sizeof(melodies::twinkle)/sizeof(melodies::twinkle[0]));
                break;
            case 2:
                sound::async_play_melody(melodies::joy, sizeof(melodies::joy)/sizeof(melodies::joy[0]));
                break;
            default:
                break;
        }

        menu::set_dirty();
    }

    void cursor_up() {
        if (cursor == 0) {
            cursor = sizeof(melodies)/sizeof(melodies[0]) - 1;
        } else {
            --cursor;
        }
        menu::set_dirty();
    }

    void cursor_down() {
        cursor = (cursor + 1) % (sizeof(melodies)/sizeof(melodies[0]));
        menu::set_dirty();
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                // Handle button presses for music app
                switch (ev.buttonEvent.button) {
                    case events::Button::UP:
                        cursor_up();
                        sound::play_navigation_tone();
                        break;
                    case events::Button::DOWN:
                        cursor_down();
                        sound::play_navigation_tone();
                        break;
                    case events::Button::A:
                        play_melody();
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
                // Handle no event (e.g., update display)
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }

    void draw(Adafruit_SSD1306& display) {
        // Draw the music app interface
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_BLACK);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(" Music Player");
        display.setTextColor(SSD1306_WHITE);
        for (size_t i = 0; i < sizeof(melodies)/sizeof(melodies[0]); ++i) {
            if (i == cursor) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.println(melodies[i]);
        }
        display.display();
    }
}