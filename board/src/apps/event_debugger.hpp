#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::event_debugger {
    void app(Adafruit_SSD1306& display);
    void draw(Adafruit_SSD1306& display);
}