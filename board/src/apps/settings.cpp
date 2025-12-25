#include <Preferences.h>
#include <nvs_flash.h>

#include "apps/settings.hpp"
#include "core/menu.hpp"



namespace apps::settings {
    menu::KBStatus kb_status;
    String input_buffer;

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        auto kb_event = menu::handle_keyboard_input(ev, kb_status, input_buffer);
        menu::upkeep(display);
    }

    void draw(Adafruit_SSD1306& display) {
        menu::draw_keyboard(display, kb_status, input_buffer);
    }
}