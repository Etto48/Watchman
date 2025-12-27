#include "apps/metronome.hpp"
#include "core/sound.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "constants.hpp"

namespace apps::metronome {
    uint16_t bpm = 120;
    sound::Note metronome_track[] = {
        { sound::NoteFrequency::NOTE_A4, 100 },
        { sound::NoteFrequency::NOTE_REST, 400 },
        { sound::NoteFrequency::NOTE_C4, 100 },
        { sound::NoteFrequency::NOTE_REST, 400 },
        { sound::NoteFrequency::NOTE_C4, 100 },
        { sound::NoteFrequency::NOTE_REST, 400 },
        { sound::NoteFrequency::NOTE_C4, 100 },
        { sound::NoteFrequency::NOTE_REST, 400 },
    };

    bool running = false;
    void change_bpm(int16_t bpm) {
        if (bpm < 40) {
            bpm = 40;
        } else if (bpm > 300) {
            bpm = 300;
        } 
        apps::metronome::bpm = bpm;
        auto period_ms = 60000 / apps::metronome::bpm;
        auto beep_duration = period_ms / 4;
        auto rest_duration = period_ms - beep_duration;
        metronome_track[0].duration = beep_duration;
        metronome_track[1].duration = rest_duration;
        metronome_track[2].duration = beep_duration;
        metronome_track[3].duration = rest_duration;
        metronome_track[4].duration = beep_duration;
        metronome_track[5].duration = rest_duration;
        metronome_track[6].duration = beep_duration;
        metronome_track[7].duration = rest_duration;
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_event.button) {
                    case events::Button::A:
                        if (running) {
                            sound::stop_async_interruptible_melody();
                            sound::play_confirm_tone();
                        } else {
                            sound::async_play_interruptible_melody(
                                metronome_track,
                                sizeof(metronome_track) / sizeof(metronome_track[0]),
                                true
                            );
                        }
                        running = !running;
                        menu::set_dirty();
                        break;
                    case events::Button::B:
                        if (running) {
                            sound::stop_async_interruptible_melody();
                            running = false;
                        }
                        sound::play_cancel_tone();
                        menu::current_app = menu::App::NONE;
                        menu::set_dirty();
                        break;
                    case events::Button::UP:
                        if (!running) {
                            sound::play_navigation_tone();
                        }
                        change_bpm(bpm + 1);
                        menu::set_dirty();
                        break;
                    case events::Button::DOWN:
                        if (!running) {
                            sound::play_navigation_tone();
                        }
                        change_bpm(bpm - 1);
                        menu::set_dirty();
                        break;
                }
                break;
            case events::EventType::NONE:
                menu::upkeep(display);
                break;
        }
    }
    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Metronome");
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(35, 20);
        display.print(bpm);
        display.setTextSize(1);
        display.setCursor(75, 26);
        display.print("BPM");
        if (running) {
            display.fillRect(10, 20, 20, 16, SSD1306_WHITE);
            display.fillRect(SCREEN_WIDTH - 30, 20, 20, 16, SSD1306_WHITE);
        } else {
            display.drawRect(10, 20, 20, 16, SSD1306_WHITE);
            display.drawRect(SCREEN_WIDTH - 30, 20, 20, 16, SSD1306_WHITE);
        }
        display.setCursor(0, 48);
        display.print("A: Start/Stop");
        display.setCursor(0, 56);
        display.print("UP/DOWN: Change BPM");
        display.display();
    }
}