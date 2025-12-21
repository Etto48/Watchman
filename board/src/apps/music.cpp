#include "apps/music.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"
#include "melodies/cmajor.hpp"
#include "melodies/twinkle.hpp"
#include "melodies/joy.hpp"
#include "images/playing_music.hpp"

namespace apps::music {
    uint64_t cursor = 0;
    bool currently_playing = false;

    constexpr const char* melodies[] = {
        "C Major Scale",
        "Twinkle",
        "Ode to Joy"
    };

    void play_melody() {
        switch (cursor) {
            case 0:
                sound::async_play_interruptible_melody(melodies::cmajor, sizeof(melodies::cmajor)/sizeof(melodies::cmajor[0]));
                break;
            case 1:
                sound::async_play_interruptible_melody(melodies::twinkle, sizeof(melodies::twinkle)/sizeof(melodies::twinkle[0]));
                break;
            case 2:
                sound::async_play_interruptible_melody(melodies::joy, sizeof(melodies::joy)/sizeof(melodies::joy[0]));
                break;
            default:
                break;
        }
        currently_playing = true;
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
                if (currently_playing) {
                    switch (ev.buttonEvent.button) {
                        case events::Button::A:
                            sound::stop_async_interruptible_melody();
                            break;
                        case events::Button::B:
                            menu::current_app = menu::App::NONE;
                            sound::play_cancel_tone();
                            menu::set_dirty();
                            break;
                        default:
                            break;
                    }
                } else {
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
                }
                
            case events::EventType::NONE:
                if (currently_playing) {
                    if (!sound::is_melody_playing()) {
                        currently_playing = false;
                        menu::set_dirty();
                    }
                }
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        if (currently_playing) {
            display.setTextSize(1);
            display.setTextColor(SSD1306_BLACK);
            display.drawBitmap(0, 0, images::playing_music, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
            display.setCursor(0, 0);
            display.println(" Playing...");
            display.setCursor(20, 54);
            display.setTextColor(SSD1306_WHITE);
            display.println("Press A to stop");
            display.display();
        } else {
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
}