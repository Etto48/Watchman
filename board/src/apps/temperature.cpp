#include <math.h>
#include <cstdint>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "apps/temperature.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace apps::temperature {
    float last_internal_temperature = NAN;
    float last_external_temperature = NAN;
    uint64_t last_update_time_us = 0;
    constexpr uint64_t update_interval_us = 5000000; // 5 seconds

    OneWire oneWire(TMP_PIN);
    DallasTemperature sensors(&oneWire);
    bool sensors_initialized = false;

    enum class TemperatureMode {
        INTERNAL_TMP,
        EXTERNAL_TMP
    };
    TemperatureMode temperature_mode = TemperatureMode::INTERNAL_TMP;

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Temperature");
        uint16_t selector_offset;
        if (temperature_mode == TemperatureMode::INTERNAL_TMP) {
            selector_offset = 0;
        } else {
            selector_offset = SCREEN_WIDTH / 2;
        }
        display.fillRect(selector_offset, 8, SCREEN_WIDTH/2, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_INVERSE);
        display.setTextSize(1);
        display.setCursor(8, 8);
        display.println("INTERNAL");
        display.setCursor(SCREEN_WIDTH - 8 - 8*6, 8);
        display.println("EXTERNAL");
        display.setTextSize(2);
        display.setCursor(24, 32);
        auto tmp = (temperature_mode == TemperatureMode::INTERNAL_TMP) ? last_internal_temperature : last_external_temperature;
        if (isnan(tmp)) {
            display.println("N/A");
        } else {
            display.print(tmp, 1);
            display.println(" C");
        }
        display.display();
    }

    float read_external_temperature_sensor() {
        if (!sensors_initialized) {
            sensors.begin();
            sensors.setWaitForConversion(false);
            sensors_initialized = true;
        }
        sensors.requestTemperatures();
        if (sensors.isConversionComplete()) {
            return sensors.getTempCByIndex(0);
        } else {
            return NAN;
        }
    }

    void update_temperatures() {
        float new_tmp;
        switch (temperature_mode) {
            case TemperatureMode::INTERNAL_TMP:
                if (isnan(last_internal_temperature) || last_update_time_us + update_interval_us <= timekeeper::now_us()) {
                    new_tmp = temperatureRead();
                    if (!isnan(new_tmp) && new_tmp != last_internal_temperature) {
                        last_internal_temperature = new_tmp;
                        last_update_time_us = timekeeper::now_us();
                        menu::set_dirty();
                    }
                }
                break;
            case TemperatureMode::EXTERNAL_TMP:
                if (isnan(last_external_temperature) || last_update_time_us + update_interval_us <= timekeeper::now_us()) {
                    new_tmp = read_external_temperature_sensor();
                    if (!isnan(new_tmp) && new_tmp != last_external_temperature) {
                        last_external_temperature = new_tmp;
                        last_update_time_us = timekeeper::now_us();
                        menu::set_dirty();
                    }
                }
                break;
        }
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_event.button) {
                    case events::Button::LEFT:
                    case events::Button::RIGHT:
                    case events::Button::A:
                        sound::play_navigation_tone();
                        if (temperature_mode == TemperatureMode::INTERNAL_TMP) {
                            temperature_mode = TemperatureMode::EXTERNAL_TMP;
                        } else {
                            temperature_mode = TemperatureMode::INTERNAL_TMP;
                        }
                        menu::set_dirty();
                        break;
                    case events::Button::B:
                        menu::current_app = menu::App::NONE;
                        sound::play_cancel_tone();
                        menu::set_dirty();
                        break;
                    default:
                        break;
                }
                break;
            case events::EventType::NONE:
                update_temperatures();
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }
}