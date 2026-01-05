#pragma once
#include <Adafruit_SSD1306.h>

namespace apps::alarm {
    void draw(Adafruit_SSD1306& display);
    void app(Adafruit_SSD1306& display);
    // Needed in order to play alarm without starting the app
    struct TimestampAndTriggered {
        time_t timestamp;
        bool triggered;
    };
    // Returns 0 if alarm is disabled or RTC not synced
    TimestampAndTriggered get_alarm_timestamp();
    void init();
}