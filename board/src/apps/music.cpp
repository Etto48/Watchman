#include "apps/music.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"
#include "melodies/cmajor.hpp"
#include "melodies/twinkle.hpp"
#include "melodies/joy.hpp"
#include "images/playing_music.hpp"
#include "core/logger.hpp"

namespace apps::music {
    size_t cursor = 0;
    bool currently_playing = false;

    constexpr const char* melodies[] = {
        "C Major Scale",
        "Twinkle",
        "Ode to Joy"
    };

    constexpr const sound::Note *melodies_notes[] = {
        melodies::cmajor,
        melodies::twinkle,
        melodies::joy
    };

    constexpr size_t melodies_lengths[] = {
        sizeof(melodies::cmajor)/sizeof(melodies::cmajor[0]),
        sizeof(melodies::twinkle)/sizeof(melodies::twinkle[0]),
        sizeof(melodies::joy)/sizeof(melodies::joy[0])
    };

    void menu_action(size_t cursor, Adafruit_SSD1306& display) {
        sound::async_play_interruptible_melody(
            melodies_notes[cursor], 
            melodies_lengths[cursor]); 
        currently_playing = true;
        logger::info("Playing melody: %s", melodies[cursor]);
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
                    menu::handle_generic_menu_navigation(ev, sizeof(melodies)/sizeof(melodies[0]), cursor, display, menu_action, false);
                }
                break;
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
            display.drawBitmap(0, 0, images::playing_music, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
            menu::draw_generic_titlebar(display, "Playing Melody");
            display.setCursor(20, 54);
            display.println("Press A to stop");
            display.display();
        } else {
            menu::draw_generic_menu(display, "Select Melody", melodies, sizeof(melodies)/sizeof(melodies[0]), cursor);
        }
    }
}