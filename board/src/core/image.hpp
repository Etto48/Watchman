#pragma once

#include <Adafruit_SSD1306.h>

namespace image {
    void display_image(const uint8_t* image_data, Adafruit_SSD1306& display);
    void print_up_arrow(Adafruit_SSD1306& display);
    void print_down_arrow(Adafruit_SSD1306& display);
}
