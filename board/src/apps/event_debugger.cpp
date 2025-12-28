#include "apps/event_debugger.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"

namespace apps::event_debugger {

    events::Event last_event = { .type = events::EventType::NONE, {} };

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        if (ev.type != events::EventType::NONE) {
            last_event = ev;
            menu::set_dirty();
        } else {
            menu::upkeep(display);
        }
    }

    const char* button_to_string(events::Button button) {
        switch (button) {
            case events::Button::A:
                return "A";
            case events::Button::B:
                return "B";
            case events::Button::UP:
                return "UP";
            case events::Button::DOWN:
                return "DOWN";
            case events::Button::LEFT:
                return "LEFT";
            case events::Button::RIGHT:
                return "RIGHT";
            default:
                return "NONE";
        }
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Event Debugger");
        display.setTextSize(1);
        display.setCursor(0, 16);
        display.setTextColor(SSD1306_WHITE);
        switch (last_event.type) {
            case events::EventType::BUTTON_PRESS:
                display.printf("Button Press:\n");
                display.printf(" Button: %s\n", 
                    button_to_string(last_event.button_press_event.button)
                );
                display.printf(" Hold: 0b%d%d%d%d%d%d%d%d\n", 
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x20) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x10) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x08) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x04) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x02) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_press_event.hold) & 0x01) ? 1 : 0,
                    0, 0
                );
                display.printf(" Timestamp: %llu\n", last_event.button_press_event.timestamp);
                display.printf(" Repeated: %s\n", last_event.button_press_event.repeated ? "Yes" : "No");
                break;
            case events::EventType::BUTTON_RELEASE:
                display.printf("Button Release:\n");
                display.printf(" Button: %s\n", 
                    button_to_string(last_event.button_release_event.button)
                );
                display.printf(" Hold: 0b%d%d%d%d%d%d%d%d\n", 
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x20) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x10) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x08) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x04) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x02) ? 1 : 0,
                    (static_cast<uint8_t>(last_event.button_release_event.hold) & 0x01) ? 1 : 0,
                    0, 0
                );
                display.printf(" Timestamp: %llu\n", last_event.button_release_event.timestamp);
                display.printf(" Duration: %llu us\n", last_event.button_release_event.duration);
                break;
            case events::EventType::NONE:
            default:
                display.printf("No events received yet.\n");
                break;
        }
        display.display();
    }
}