#include "apps/battery.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/battery.hpp"
#include "constants.hpp"

namespace apps::battery {
    uint8_t voltage_dv = 0; // voltage decivolts
    void app(Adafruit_SSD1306 &display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::B:
                        menu::current_app = menu::App::NONE;
                        menu::set_dirty();
                        break;
                    default:
                        break;
                }
                break;
            case events::EventType::NONE:
            {
                auto status = ::battery::get_battery_status();
                if (status.voltage_dv != voltage_dv) {
                    voltage_dv = status.voltage_dv;
                    menu::set_dirty();
                }
                menu::upkeep(display);
                break;
            }
        }
    }
    void draw(Adafruit_SSD1306 &display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Battery");
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH / 2 - 2*6*2, 30);
        display.printf("%u.%uV", voltage_dv / 10, voltage_dv % 10);
        display.display();
    }
}