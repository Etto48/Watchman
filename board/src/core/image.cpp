#include <cstdint>

#include "core/image.hpp"
#include "constants.hpp"

namespace image {
    void display_image(const uint8_t* image_data, Adafruit_SSD1306& display) {
        display.clearDisplay();
        display.drawBitmap(0, 0, image_data, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        display.display();
    }
    void print_up_arrow(Adafruit_SSD1306& display) {
        constexpr uint8_t up_arrow[] = {
            0b00011000,
            0b00111100,
            0b01111110};
        display.drawBitmap(display.getCursorX(), display.getCursorY(), up_arrow, 8, 3, SSD1306_WHITE);
    }
    void print_down_arrow(Adafruit_SSD1306& display) {
        constexpr uint8_t down_arrow[] = {
            0b01111110,
            0b00111100,
            0b00011000};
        display.drawBitmap(display.getCursorX(), display.getCursorY(), down_arrow, 8, 3, SSD1306_WHITE);
    }
}