#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::deepsleep {
    void app(Adafruit_SSD1306& display);

    void draw(Adafruit_SSD1306& display);
}