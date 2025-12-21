#include <math.h>
#include <cstdint>

#include "apps/temperature.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"

namespace apps::temperature {
    float last_temperature = NAN;

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        display.setTextSize(1);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(0, 0);
        display.println(" Temperature");
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(24, 28);
        if (isnan(last_temperature)) {
            display.println("N/A");
        } else {
            display.print(last_temperature, 1);
            display.println(" C");
        }
        display.display();
    }

    void app(Adafruit_SSD1306& display) {
        static uint64_t last_update_time = 0;
        constexpr uint64_t update_interval_ms = 5000;
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
                if (isnan(last_temperature) || millis() - last_update_time >= update_interval_ms) {
                    last_temperature = temperatureRead();
                    last_update_time = millis();
                    menu::set_dirty();
                }
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }
}