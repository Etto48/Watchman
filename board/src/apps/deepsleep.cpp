#include "apps/deepsleep.hpp"
#include "core/deepsleep.hpp"
#include "core/menu.hpp"

namespace apps::deepsleep {
    void app(Adafruit_SSD1306& display) {
        ::deepsleep::deepsleep(display);
        menu::current_app = menu::App::NONE; // Prevent re-entry into deepsleep
    }

    void draw(Adafruit_SSD1306& display) {
        // No drawing needed for deepsleep app
    }
}