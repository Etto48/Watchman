#include <cstdint>

#include "core/image.hpp"
#include "constants.hpp"

void display_image(const uint8_t* image_data, Adafruit_SSD1306& display) {
    display.clearDisplay();
    display.drawBitmap(0, 0, image_data, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
}