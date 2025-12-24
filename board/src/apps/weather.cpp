#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "apps/weather.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/logger.hpp"
#include "certs/isrg_root_x1.hpp"
#include "constants.hpp"
#include "private.hpp"
#include "images/weather_broken_clouds.hpp"
#include "images/weather_clear.hpp"
#include "images/weather_few_clouds.hpp"
#include "images/weather_shower_rain.hpp"
#include "images/weather_rain.hpp"
#include "images/weather_scattered_clouds.hpp"
#include "images/weather_snow.hpp"
#include "images/weather_mist.hpp"
#include "images/weather_thunderstorm.hpp"
#include "images/weather_unknown.hpp"

namespace apps::weather {
    TaskHandle_t weather_update_task_handle = nullptr;
    SemaphoreHandle_t weather_data_mutex = nullptr;
    float latest_temperature = NAN;
    WeatherCode latest_weather_code = WeatherCode::UNKNOWN;
    
    float latest_temperature_local = NAN;
    WeatherCode latest_weather_code_local = WeatherCode::UNKNOWN;

    enum class WeatherTaskCommand {
        PAUSE = 1 << 0,
        RESUME = 1 << 1,
    };

    constexpr uint64_t weather_update_interval_on_failure_ms = 10 * 1000; // Retry every 10 seconds on failure
    constexpr uint64_t weather_update_interval_ms = 10 * 60 * 1000; // Update every 10 minutes

    void update_weather_task(void* param) {
        constexpr const char* weather_api_url = "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current_weather=true";
        char url_buffer[256];
        snprintf(url_buffer, sizeof(url_buffer), weather_api_url, LATITUDE, LONGITUDE);
        url_buffer[sizeof(url_buffer) - 1] = '\0';
        WiFiClientSecure client;
        client.setCACert(certs::ISRG_ROOT_X1_CA);
        HTTPClient https;
        bool at_least_one_success = false;
        while (true) {
            uint32_t notification_value = 0;
            if (xTaskNotifyWait(0, UINT32_MAX, &notification_value, 0) == pdTRUE) {
                if (notification_value & static_cast<uint32_t>(WeatherTaskCommand::PAUSE)) {
                    // Wait indefinitely until resumed
                    xTaskNotifyWait(0, UINT32_MAX, &notification_value, portMAX_DELAY);
                    if (notification_value & static_cast<uint32_t>(WeatherTaskCommand::RESUME)) {
                        // Resumed, continue
                        continue;
                    }
                }
            }


            https.begin(client, url_buffer);
            int httpCode = https.GET();
            if (httpCode > 0) {
                if (httpCode == HTTP_CODE_OK) {
                    String payload = https.getString();
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, payload);
                    if (!error) {
                        float temperature = doc["current_weather"]["temperature"] | NAN;
                        int weather_code_int = doc["current_weather"]["weathercode"] | -1;
                        WeatherCode weather_code = WeatherCode::UNKNOWN;
                        if (weather_code_int >= 0 && weather_code_int <= 99) {
                            weather_code = static_cast<WeatherCode>(weather_code_int);
                        }
                        if (xSemaphoreTake(weather_data_mutex, pdMS_TO_TICKS(portMAX_DELAY))) {
                            latest_temperature = temperature;
                            latest_weather_code = weather_code;
                            xSemaphoreGive(weather_data_mutex);
                            at_least_one_success = true;
                        }
                    } else {
                        logger::error("Failed to parse weather JSON: %s", error.c_str());
                    }
                } else {
                    logger::error("Unexpected HTTP code from weather API: %d", httpCode);
                }
            } else {
                logger::error("Failed to connect to weather API: %s", https.errorToString(httpCode).c_str());
            }
            https.end();
            if (!at_least_one_success) {
                vTaskDelay(pdMS_TO_TICKS(weather_update_interval_on_failure_ms));
            } else {
                vTaskDelay(pdMS_TO_TICKS(weather_update_interval_ms));
            }
        }
    }

    void pause_weather_task() {
        if (weather_update_task_handle != nullptr) {
            xTaskNotify(weather_update_task_handle, static_cast<uint32_t>(WeatherTaskCommand::PAUSE), eSetValueWithOverwrite);
        }
    }

    void resume_weather_task() {
        if (weather_update_task_handle != nullptr) {
            xTaskNotify(weather_update_task_handle, static_cast<uint32_t>(WeatherTaskCommand::RESUME), eSetValueWithOverwrite);
        }
    }

    void try_start_weather_task() {
        if (weather_update_task_handle == nullptr) {
            weather_data_mutex = xSemaphoreCreateMutex();
            xTaskCreate(
                update_weather_task,
                "WeatherUpdateTask",
                8192,
                nullptr,
                1,
                &weather_update_task_handle
            );
        }
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.buttonEvent.button) {
                    case events::Button::B:
                        sound::play_cancel_tone();
                        menu::current_app = menu::App::NONE;
                        menu::set_dirty();
                        break;
                }
                break;
            case events::EventType::NONE:
                try_start_weather_task();
                if (weather_data_mutex != nullptr && xSemaphoreTake(weather_data_mutex, 0) == pdTRUE) {
                    if ((!isnan(latest_temperature) && latest_temperature != latest_temperature_local) || (latest_weather_code != latest_weather_code_local)) {
                        latest_temperature_local = latest_temperature;
                        latest_weather_code_local = latest_weather_code;
                        menu::set_dirty();
                    }
                    xSemaphoreGive(weather_data_mutex);
                }
                menu::upkeep(display);
                break;
        }
    }

    WeatherCondition condition_from_weather_code(WeatherCode code) {
        switch (code) {
            case WeatherCode::CLEAR_SKY:
                return WeatherCondition::CLEAR;
            case WeatherCode::MAINLY_CLEAR:
                return WeatherCondition::FEW_CLOUDS;
            case WeatherCode::PARTLY_CLOUDY:
                return WeatherCondition::SCATTERED_CLOUDS;
            case WeatherCode::OVERCAST:
                return WeatherCondition::BROKEN_CLOUDS;
            case WeatherCode::LIGHT_DRIZZLE:
            case WeatherCode::MODERATE_DRIZZLE:
            case WeatherCode::DENSE_DRIZZLE:
            case WeatherCode::SLIGHT_RAIN:
            case WeatherCode::MODERATE_RAIN:
            case WeatherCode::HEAVY_RAIN:
            case WeatherCode::SLIGHT_RAIN_SHOWERS:
            case WeatherCode::MODERATE_RAIN_SHOWERS:
            case WeatherCode::VIOLENT_RAIN_SHOWERS:
                return WeatherCondition::RAIN;
            case WeatherCode::THUNDERSTORM:
            case WeatherCode::THUNDERSTORM_WITH_SLIGHT_HAIL:
            case WeatherCode::THUNDERSTORM_WITH_HEAVY_HAIL:
                return WeatherCondition::THUNDERSTORM;
            case WeatherCode::SLIGHT_SNOWFALL:
            case WeatherCode::MODERATE_SNOWFALL:
            case WeatherCode::HEAVY_SNOWFALL:
            case WeatherCode::SLIGHT_SNOW_SHOWERS:
            case WeatherCode::HEAVY_SNOW_SHOWERS:
                return WeatherCondition::SNOW;
            case WeatherCode::FOG:
            case WeatherCode::DEPOSITING_RIME_FOG:
                return WeatherCondition::MIST;
            default:
                return WeatherCondition::UNKNOWN;
        }
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Weather");
        const uint8_t* weather_icon = nullptr;
        WeatherCondition condition = condition_from_weather_code(latest_weather_code_local);
        switch (condition) {
            case WeatherCondition::CLEAR:
                weather_icon = images::weather_clear;
                break;
            case WeatherCondition::FEW_CLOUDS:
                weather_icon = images::weather_few_clouds;
                break;
            case WeatherCondition::SCATTERED_CLOUDS:
                weather_icon = images::weather_scattered_clouds;
                break;
            case WeatherCondition::BROKEN_CLOUDS:
                weather_icon = images::weather_broken_clouds;
                break;
            case WeatherCondition::SHOWER_RAIN:
                weather_icon = images::weather_shower_rain;
                break;
            case WeatherCondition::RAIN:
                weather_icon = images::weather_rain;
                break;
            case WeatherCondition::THUNDERSTORM:
                weather_icon = images::weather_thunderstorm;
                break;
            case WeatherCondition::SNOW:
                weather_icon = images::weather_snow;
                break;
            case WeatherCondition::MIST:
                weather_icon = images::weather_mist;
                break;
            default:
                weather_icon = images::weather_unknown;
                break;
        }
        display.drawBitmap(16, 20, weather_icon, images::weather_clear_width, images::weather_clear_height, SSD1306_WHITE);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        if (!isnan(latest_temperature_local)) {    
            display.setCursor(48, 32);
            display.printf("%.1fC", latest_temperature_local);
        } else {
            display.setCursor(48, 32);
            display.print("N/A");
        }
        display.display();
    }
}