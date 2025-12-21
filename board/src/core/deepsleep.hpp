#pragma once

#include <Adafruit_SSD1306.h>

namespace deepsleep {
    // Put the device into deep sleep mode after displaying a message
    void deepsleep(Adafruit_SSD1306& display);
}
