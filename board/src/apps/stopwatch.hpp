#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::stopwatch {
    void draw(Adafruit_SSD1306& display);
    void app(Adafruit_SSD1306& display);
}