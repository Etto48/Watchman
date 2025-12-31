#include "apps/pet.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "images/pet_happy.hpp"
#include "constants.hpp"

namespace apps::pet {
    enum class PetState {
        NORMAL,
        HAPPY,
        SAD,
        SLEEPING,
        HUNGRY,
        ANGRY,
    };

    PetState current_state = PetState::HAPPY;
    uint16_t current_frame = 0;
    uint16_t repeat_count = 0;
    uint16_t repeat_until = 10;
    uint64_t last_update_ms = 0;

    void update() {
        constexpr uint64_t FPS = 24;
        constexpr uint64_t FRAME_DURATION_MS = 1000 / FPS;
        auto now = timekeeper::now_us() / 1000;
        if (last_update_ms + FRAME_DURATION_MS < now) {
            last_update_ms = now;
            if (current_frame == 0) {
                if (repeat_count < repeat_until) {
                    repeat_count++;
                    return;
                } else {
                    repeat_count = 0;
                }
            }
            current_frame++;
            menu::set_dirty();
            current_frame %= images::pet_happy_width / SCREEN_WIDTH;
            if (current_frame == 0) {
                repeat_until = random(5, 40);
            }
        }
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::B:
                        sound::play_cancel_tone();
                        menu::current_app = menu::App::NONE;
                        menu::set_dirty();
                        break;
                }
                break;
            case events::EventType::NONE:
                update();
                menu::upkeep(display);
                break;
        }
    }  
    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
            for (uint16_t y = 0; y < SCREEN_HEIGHT; ++y) {
                uint16_t src_x = (current_frame * SCREEN_WIDTH + x) / 8;
                uint16_t src_y = y;
                bool pixel = images::pet_happy[src_y * (images::pet_happy_width / 8) + src_x] & (1 << (7 - (x % 8)));
                display.drawPixel(x, y, pixel ? SSD1306_WHITE : SSD1306_BLACK);
            }
        }              
        display.display();
    }
}