#pragma once

#include <Adafruit_SSD1306.h>

namespace apps::clock {
    void draw(Adafruit_SSD1306& display);
    void app(Adafruit_SSD1306& display);
}