#include <cstdint>

#include "core/deepsleep.hpp"
#include "core/events.hpp"
#include "images/deepsleep.hpp"
#include "constants.hpp"
#include "core/image.hpp"
#include "core/sound.hpp"
#include "core/jingle.hpp"
#include "core/logger.hpp"
#include "core/menu.hpp"
#include "deepsleep.hpp"

namespace deepsleep {
    void deepsleep(Adafruit_SSD1306& display) {
        sound::stop_async_interruptible_melody(); // Stop any playing melody
        image::display_image(images::deepsleep, display);
        sound::play_melody(deepsleep_jingle_melody, sizeof(deepsleep_jingle_melody)/sizeof(deepsleep_jingle_melody[0]));
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        gpio_wakeup_enable(static_cast<gpio_num_t>(A_PIN), GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        esp_sleep_enable_timer_wakeup(DEEPSLEEP_GRACE_PERIOD_US);
        logger::info("Entering light-sleep.");
        esp_light_sleep_start();
        auto wakeup_cause = esp_sleep_get_wakeup_cause();
        if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
            logger::info("Grace period expired.");
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            esp_deep_sleep_enable_gpio_wakeup(1 << A_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
            logger::info("Entering deep-sleep.");
            esp_deep_sleep_start();
        } else if (wakeup_cause == ESP_SLEEP_WAKEUP_GPIO) {
            logger::info("Deep-sleep aborted by GPIO wakeup.");
        } else {
            logger::info("Woke up from unknown reason, continuing execution.");
        }
        events::clear_event_queue(); // Avoid sending the button press event that woke us up
        events::update_last_event_timestamp(); // Prevent immediate re-entry into deepsleep
        display.ssd1306_command(SSD1306_DISPLAYON);
        sound::play_cancel_tone();
        menu::set_dirty();
        return;
    }
}