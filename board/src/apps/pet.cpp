#include "apps/pet.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "images/pet_happy.hpp"

namespace apps::pet {
    enum class PetState {
        NORMAL,
        HAPPY,
        SAD,
        SLEEPING,
        HUNGRY,
        ANGRY,
    };


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
                menu::upkeep(display);
                break;
        }
    }  
    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        display.drawBitmap(0, 0, images::pet_happy, images::pet_happy_width, images::pet_happy_height, SSD1306_WHITE);
        display.display();
    }
}