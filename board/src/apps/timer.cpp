#include "apps/timer.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace apps::timer {
    constexpr uint64_t ONE_MINUTE_US = 60 * 1000000;
    constexpr uint64_t ONE_SECOND_US = 1000000;
    constexpr uint64_t MAX_TIMER_US = 59 * ONE_MINUTE_US + 59 * ONE_SECOND_US;
    uint64_t timer_duration_us = 5 * ONE_MINUTE_US;
    uint64_t remaining_time_us = timer_duration_us;
    uint64_t start_time_us = 0;
    uint64_t last_update_time_us = 0;

    constexpr sound::Note alert_melody[] = {
        { sound::NoteFrequency::NOTE_C5, 300 },
        { sound::NoteFrequency::NOTE_E5, 150 },
        { sound::NoteFrequency::NOTE_G5, 150 },
        { sound::NoteFrequency::NOTE_B5, 300 },
        { sound::NoteFrequency::NOTE_D5, 300 },
        { sound::NoteFrequency::NOTE_F5, 150 },
        { sound::NoteFrequency::NOTE_A5, 150 },
        { sound::NoteFrequency::NOTE_C6, 300 },
    };

    enum class TimerState {
        IDLE,
        EDITING,
        RUNNING,
        PAUSED,
        FINISHED,
    };
    enum class TimerField {
        MINUTES,
        SECONDS,
    };
    TimerState timer_state = TimerState::IDLE;
    TimerField timer_field = TimerField::MINUTES;

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (timer_state) {
                    case TimerState::IDLE:
                        switch (ev.buttonEvent.button) {
                            case events::Button::A:
                                sound::play_confirm_tone();
                                timer_state = TimerState::RUNNING;
                                start_time_us = timekeeper::now_us();
                                menu::set_dirty();
                                break;
                            case events::Button::B:
                                sound::play_cancel_tone();
                                menu::current_app = menu::App::NONE;
                                menu::set_dirty();
                                break;
                            case events::Button::UP:
                            case events::Button::DOWN:
                            case events::Button::LEFT:
                            case events::Button::RIGHT:
                                sound::play_navigation_tone();
                                timer_state = TimerState::EDITING;
                                menu::set_dirty();
                                break;
                            default:
                                break;
                        }
                        break;
                    case TimerState::EDITING:
                        switch (ev.buttonEvent.button) {
                            case events::Button::A:
                                sound::play_confirm_tone();
                                timer_state = TimerState::IDLE;
                                menu::set_dirty();
                                break;
                            case events::Button::B:
                                sound::play_confirm_tone();
                                timer_state = TimerState::IDLE;
                                menu::set_dirty();
                                break;
                            case events::Button::UP:
                                sound::play_navigation_tone();
                                if (timer_field == TimerField::MINUTES) {
                                    timer_duration_us += ONE_MINUTE_US;
                                } else {
                                    timer_duration_us += ONE_SECOND_US;
                                }
                                if (timer_duration_us > MAX_TIMER_US) {
                                    if (timer_field == TimerField::MINUTES) {
                                        timer_duration_us = ONE_MINUTE_US;
                                    } else {
                                        timer_duration_us = ONE_SECOND_US;
                                    }
                                }
                                remaining_time_us = timer_duration_us;
                                menu::set_dirty();
                                break;
                            case events::Button::DOWN:
                                sound::play_navigation_tone();
                                {
                                    uint64_t decrement;
                                    if (timer_field == TimerField::MINUTES) {
                                        decrement = ONE_MINUTE_US;
                                    } else {
                                        decrement = ONE_SECOND_US;
                                    }
                                    if (timer_duration_us > decrement) {
                                        timer_duration_us -= decrement;
                                    } else {
                                        if (timer_field == TimerField::MINUTES) {
                                            timer_duration_us = MAX_TIMER_US - ONE_MINUTE_US + ONE_SECOND_US;
                                        } else {
                                            timer_duration_us = MAX_TIMER_US - ONE_SECOND_US;
                                        }
                                    }
                                }
                                remaining_time_us = timer_duration_us;
                                menu::set_dirty();
                                break;
                            case events::Button::LEFT:
                            case events::Button::RIGHT:
                                sound::play_navigation_tone();
                                if (timer_field == TimerField::SECONDS) {
                                    timer_field = TimerField::MINUTES;
                                } else {
                                    timer_field = TimerField::SECONDS;
                                }
                                menu::set_dirty();
                                break;
                            default:
                                break;
                        }
                        break;
                    case TimerState::RUNNING:
                        switch (ev.buttonEvent.button) {
                            case events::Button::A:
                            case events::Button::B:
                                sound::play_confirm_tone();
                                timer_state = TimerState::PAUSED;
                                {
                                    auto now = timekeeper::now_us();
                                    if (now < start_time_us) { // Finished but state not yet updated
                                        remaining_time_us = timer_duration_us;
                                        timer_state = TimerState::IDLE;
                                    } else {
                                        remaining_time_us -= now - start_time_us;
                                    }
                                }
                                menu::set_dirty();
                                break;
                            default:
                                break;
                        }
                        break;
                    case TimerState::PAUSED:
                        switch (ev.buttonEvent.button) {
                            case events::Button::A:
                                sound::play_confirm_tone();
                                timer_state = TimerState::RUNNING;
                                start_time_us = timekeeper::now_us();
                                menu::set_dirty();
                                break;
                            case events::Button::B:
                                sound::play_cancel_tone();
                                remaining_time_us = timer_duration_us;
                                timer_state = TimerState::IDLE;
                                menu::set_dirty();
                                break;
                            default:
                                break;
                        }
                        break;
                    case TimerState::FINISHED:
                        switch (ev.buttonEvent.button) {
                            case events::Button::A:
                            case events::Button::B:
                                sound::stop_async_interruptible_melody();
                                remaining_time_us = timer_duration_us;
                                timer_state = TimerState::IDLE;
                                menu::set_dirty();
                                break;
                            default:
                                break;
                        }
                        break;
                }
            case events::EventType::NONE:
            {
                auto now = timekeeper::now_us();
                if (timer_state == TimerState::RUNNING) {
                    uint64_t elapsed_us = now - start_time_us;
                    if (elapsed_us >= remaining_time_us) {
                        timer_state = TimerState::FINISHED;
                        sound::async_play_interruptible_melody(alert_melody, sizeof(alert_melody) / sizeof(alert_melody[0]), true);
                        menu::set_dirty();
                    } else if (last_update_time_us + ONE_SECOND_US <= now) {
                        last_update_time_us = now;
                        menu::set_dirty(); // Continuously update while running
                    }
                } else if (timer_state == TimerState::FINISHED && last_update_time_us + ONE_SECOND_US <= now) {
                    last_update_time_us = now;
                    menu::set_dirty(); // Continuously update while finished
                }   
                menu::upkeep(display);
                break;
            }
            default:
                break;
        }  
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Timer");

        uint64_t display_time_us;
        uint8_t background_fill_width = 0;
        auto now = timekeeper::now_us();
        if (timer_state == TimerState::RUNNING) {
            uint64_t elapsed_us = now - start_time_us;
            if (elapsed_us >= remaining_time_us) {
                display_time_us = 0;
            } else {
                display_time_us = remaining_time_us - elapsed_us;
            }
            background_fill_width = (SCREEN_WIDTH * (timer_duration_us - display_time_us)) / timer_duration_us;
        } else if (timer_state == TimerState::FINISHED) {
            uint64_t elapsed_us = now - start_time_us;
            display_time_us = elapsed_us - remaining_time_us;
            background_fill_width = SCREEN_WIDTH;
        } else {
            display_time_us = remaining_time_us;
        }

        uint64_t total_seconds = display_time_us / ONE_SECOND_US;
        uint64_t minutes = total_seconds / 60;
        uint64_t seconds = total_seconds % 60;

        display.setTextSize(2);
        display.setTextColor(SSD1306_INVERSE);
        
        display.fillRect(0, 18, background_fill_width, 18, SSD1306_WHITE);
        
        display.setCursor(30, 20);
        if (timer_field == TimerField::MINUTES && timer_state == TimerState::EDITING) {
            display.fillRect(28, 18, 26, 18, SSD1306_WHITE);
        }
        display.printf("%02llu", minutes);
        display.print(':');
        if (timer_field == TimerField::SECONDS && timer_state == TimerState::EDITING) {
            display.fillRect(64, 18, 26, 18, SSD1306_WHITE);
        }
        display.printf("%02llu", seconds);
        display.setTextColor(SSD1306_WHITE);

        display.setTextSize(1);
        display.setCursor(10, 50);
        switch (timer_state) {
            case TimerState::IDLE:
                display.println("Start: A, Edit: +");
                break;
            case TimerState::EDITING:
                display.println("Edit: +, Save: A/B");
                break;
            case TimerState::RUNNING:
                display.println("Pause: A, Reset: B");
                break;
            case TimerState::PAUSED:
                display.println("Resume: A, Reset: B");
                break;
            case TimerState::FINISHED:
                display.println("Reset: A/B");
                break;
        }

        display.display();
    }
}