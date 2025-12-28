#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "apps/weather.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "core/logger.hpp"
#include "apps/settings.hpp"
#include "certs/isrg_root_x1.hpp"
#include "constants.hpp"
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
    String latest_location = "Unknown";
    
    float latest_temperature_local = NAN;
    WeatherCode latest_weather_code_local = WeatherCode::UNKNOWN;
    String latest_location_local = "Unknown";

    enum class WeatherTaskCommand {
        PAUSE = 1 << 0,
        RESUME = 1 << 1,
    };

    constexpr uint64_t weather_update_interval_on_failure_ms = 10 * 1000; // Retry every 10 seconds on failure
    constexpr uint64_t weather_update_interval_ms = 10 * 60 * 1000; // Update every 10 minutes

    void update_weather_task(void* param) {
        constexpr const char* weather_api_url = "https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current_weather=true";
        constexpr const char* geo_api_url = "https://nominatim.openstreetmap.org/search?q=%s&format=json&limit=1";
        char url_buffer[256];
        float latitude = 0.0f;
        float longitude = 0.0f;
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
            if (!WiFi.isConnected()) {
                vTaskDelay(pdMS_TO_TICKS(weather_update_interval_on_failure_ms));
                continue;
            }

            bool location_success = false;
            auto settings = apps::settings::get_settings();
            snprintf(url_buffer, sizeof(url_buffer) - 1, geo_api_url, settings.location.c_str());
            url_buffer[sizeof(url_buffer) - 1] = '\0';
            https.begin(client, url_buffer);
            int geo_http_code = https.GET();
            if (geo_http_code > 0) {
                if (geo_http_code == HTTP_CODE_OK) {
                    String geo_payload = https.getString();
                    JsonDocument geo_doc;
                    DeserializationError geo_error = deserializeJson(geo_doc, geo_payload);
                    if (!geo_error) {
                        if (geo_doc.is<JsonArray>() && geo_doc.size() > 0) {
                            latitude = geo_doc[0]["lat"];
                            longitude = geo_doc[0]["lon"];
                            String location_name = geo_doc[0]["display_name"].as<String>();
                            if (xSemaphoreTake(weather_data_mutex, pdMS_TO_TICKS(portMAX_DELAY))) {
                                latest_location = location_name;
                                xSemaphoreGive(weather_data_mutex);
                            }
                            location_success = true;
                        } else {
                            logger::error("Failed to get geocoding data: unexpected JSON structure");
                        }
                    } else {
                        logger::error("Failed to parse geocoding JSON: %s", geo_error.c_str());
                    }
                } else {
                    logger::error("Unexpected HTTP code from geocoding API: %d", geo_http_code);
                }
            } else {
                logger::error("Failed to connect to geocoding API: %s", https.errorToString(geo_http_code).c_str());
            }
            https.end();
            if (!location_success) {
                vTaskDelay(pdMS_TO_TICKS(weather_update_interval_on_failure_ms));
                continue;
            }

            snprintf(url_buffer, sizeof(url_buffer) - 1, weather_api_url, latitude, longitude);
            url_buffer[sizeof(url_buffer) - 1] = '\0';

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
                switch (ev.button_press_event.button) {
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
                        latest_location_local = latest_location;
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
        display.drawBitmap(16, 8, weather_icon, images::weather_clear_width, images::weather_clear_height, SSD1306_WHITE);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        if (!isnan(latest_temperature_local)) {    
            display.setCursor(48, 20);
            display.printf("%.1fC", latest_temperature_local);
        } else {
            display.setCursor(48, 20);
            display.print("N/A");
        }
        display.setTextSize(1);
        display.setCursor(0, 40);
        auto lines = latest_location_local;
        lines.replace(", ", "\n");
        display.print(lines);
        display.display();
    }
}