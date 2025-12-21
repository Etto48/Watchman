#include <cstdint>

#include "core/deepsleep.hpp"
#include "core/events.hpp"
#include "images/deepsleep.hpp"
#include "constants.hpp"
#include "core/image.hpp"
#include "core/sound.hpp"
#include "core/jingle.hpp"
#include "core/logger.hpp"

namespace deepsleep {
    void deepsleep(Adafruit_SSD1306& display) {
        display_image(images::deepsleep, display);
        sound::play_melody(deepsleep_jingle_melody, sizeof(deepsleep_jingle_melody)/sizeof(deepsleep_jingle_melody[0]));
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        esp_deep_sleep_enable_gpio_wakeup(1 << A_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
        logger::info("Entering deep-sleep.");
        esp_deep_sleep_start();
    }
}