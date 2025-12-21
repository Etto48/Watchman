#include <cstdint>

#include "core/menu.hpp"
#include "core/events.hpp"
#include "core/sound.hpp"
#include "apps/temperature.hpp"
#include "apps/music.hpp"
#include "core/deepsleep.hpp"
#include "constants.hpp"
#include "images/wifi_0.hpp"
#include "images/wifi_1.hpp"
#include "images/wifi_2.hpp"
#include "images/wifi_3.hpp"
#include "images/battery_0.hpp"
#include "images/battery_1.hpp"
#include "images/battery_2.hpp"
#include "images/battery_3.hpp"
#include "apps/time.hpp"
#include "core/wifi.hpp"
#include "core/battery.hpp"
#include "menu.hpp"

namespace menu {
    static SemaphoreHandle_t status_mutex = nullptr;
    bool dirty = true;
    wifi::WiFiStatus last_wifi_status = wifi::WiFiStatus::DISCONNECTED;
    battery::BatteryLevel last_battery_level = battery::BatteryLevel::BATTERY_EMPTY;
    
    App current_app = App::NONE;
    size_t cursor = 0;

    constexpr const char *menu_items[] = {
        "Temperature",
        "Time",
        "Music",
        "Deepsleep"
    };

    void draw_wifi_icon(Adafruit_SSD1306& display) {
        const uint8_t* wifi_icon = nullptr;
        switch (last_wifi_status) {
            case wifi::WiFiStatus::DISCONNECTED:
                wifi_icon = images::wifi_0;
                break;
            case wifi::WiFiStatus::CONNECTED_WEAK:
                wifi_icon = images::wifi_1;
                break;
            case wifi::WiFiStatus::CONNECTED_AVERAGE:
                wifi_icon = images::wifi_2;
                break;
            case wifi::WiFiStatus::CONNECTED_STRONG:
                wifi_icon = images::wifi_3;
                break;
            default:
                wifi_icon = images::wifi_0;
                break;
        }
        display.drawBitmap(SCREEN_WIDTH - 8, 0, wifi_icon, 8, 8, SSD1306_BLACK);
    }

    void draw_battery_icon(Adafruit_SSD1306 &display) {
        const uint8_t* battery_icon = nullptr;
        switch (last_battery_level) {
            case battery::BatteryLevel::BATTERY_EMPTY:
                battery_icon = images::battery_0;
                break;
            case battery::BatteryLevel::BATTERY_LOW:
                battery_icon = images::battery_1;
                break;
            case battery::BatteryLevel::BATTERY_MEDIUM:
                battery_icon = images::battery_2;
                break;
            case battery::BatteryLevel::BATTERY_FULL:
                battery_icon = images::battery_3;
                break;
            default:
                battery_icon = images::battery_0;
                break;
        }
        display.drawBitmap(SCREEN_WIDTH - 16, 0, battery_icon, 8, 8, SSD1306_BLACK);
    }

    void draw_main_menu(Adafruit_SSD1306 &display) {
        display.clearDisplay();
        display.setTextSize(1);
        display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(0, 0);
        display.println(" Main Menu");
        draw_battery_icon(display);
        draw_wifi_icon(display);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        size_t len = sizeof(menu_items)/sizeof(menu_items[0]);
        for (size_t i = 0; i < len; ++i) {
            if (i == cursor) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.println(menu_items[i]);
        }
        display.display();
    }

    void status_update_task(void* param) {
        while (true) {
            wifi::WiFiStatus current_wifi_status = wifi::get_status();
            battery::BatteryLevel current_battery_level = battery::get_battery_level();
            if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
                if (current_wifi_status != last_wifi_status) {
                    last_wifi_status = current_wifi_status;
                    dirty = true;
                }
                if (current_battery_level != last_battery_level) {
                    last_battery_level = current_battery_level;
                    dirty = true;
                }
                xSemaphoreGive(status_mutex);
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); // Update every 1 seconds
        }
    }

    void init() {
        configTime(GMT_OFFSET_S, DAYLIGHT_OFFSET_S, "pool.ntp.org", "time.nist.gov");
        status_mutex = xSemaphoreCreateMutex();
        xTaskCreate(status_update_task, "StatusUpdate", 2048, nullptr, 1, nullptr);
    }

    void upkeep(Adafruit_SSD1306& display) {
        bool needs_redraw = false;
        if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
            needs_redraw = dirty;
            dirty = false;
            xSemaphoreGive(status_mutex);
        }
        if (needs_redraw) {
            switch (current_app) {
                case App::NONE:
                    draw_main_menu(display);
                    break;
                case App::TEMPERATURE:
                    apps::temperature::draw(display);
                    break;
                case App::TIME:
                    apps::time::draw(display);
                    break;
                case App::MUSIC:
                    apps::music::draw(display);
                    break;
                default:
                    break;
            }
        }
        auto last_event_timestamp = events::get_last_event_timestamp();
        if (last_event_timestamp + TIME_BEFORE_DEEPSLEEP_MS < millis()) {
            deepsleep::deepsleep(display);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent busy looping
    }

    void cursor_up() {
        if (cursor == 0) {
            cursor = sizeof(menu_items)/sizeof(menu_items[0]) - 1;
        } else {
            cursor--;
        }
    }

    void cursor_down() {
        cursor = (cursor + 1) % (sizeof(menu_items)/sizeof(menu_items[0]));
    }

    void main_menu(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.buttonEvent.button) {
                    case events::Button::UP:
                        cursor_up();
                        sound::play_navigation_tone();
                        set_dirty();
                        break;
                    case events::Button::DOWN:
                        cursor_down();
                        sound::play_navigation_tone();
                        set_dirty();
                        break;
                    case events::Button::A:
                    switch (cursor) {
                            case 0:
                                sound::play_confirm_tone();
                                current_app = App::TEMPERATURE;
                                set_dirty();
                                break;
                            case 1:
                                sound::play_confirm_tone();
                                current_app = App::TIME;
                                set_dirty();
                                break;
                            case 2:
                                sound::play_confirm_tone();
                                current_app = App::MUSIC;
                                set_dirty();
                                break;
                            case 3:
                                deepsleep::deepsleep(display);
                                break;
                            default:
                                current_app = App::NONE;
                                break;
                        }
                    default:
                        break;
                }
                break;
            case events::EventType::NONE:
                upkeep(display);
                break;
            default:
                break;
        }
    }

    void set_dirty()
    {
        if (xSemaphoreTake(status_mutex, portMAX_DELAY)) {
            dirty = true;
            xSemaphoreGive(status_mutex);
        }
    }

    void main_loop(Adafruit_SSD1306 &display)
    {
        switch (current_app) {
            case App::NONE:
                main_menu(display);
                break;
            case App::TEMPERATURE:
                apps::temperature::app(display);
                break;
            case App::TIME:
                apps::time::app(display);
                break;
            case App::MUSIC:
                apps::music::app(display);
                break;
            default:
                break;
        }
    }
}