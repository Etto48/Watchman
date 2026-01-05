#include <WiFi.h>

#include "core/wifi.hpp"
#include "apps/settings.hpp"

namespace wifi {
    WiFiStatus get_status() {
        apps::settings::Settings settings = apps::settings::get_settings();
        if (!settings.wifi_enabled) {
            return WiFiStatus::DISABLED_BY_USER;
        }
        wl_status_t status = WiFi.status();
        if (status != WL_CONNECTED) {
            return WiFiStatus::DISCONNECTED;
        }
        int32_t rssi = WiFi.RSSI();
        if (rssi >= -60) {
            return WiFiStatus::CONNECTED_STRONG;
        } else if (rssi >= -75) {
            return WiFiStatus::CONNECTED_AVERAGE;
        } else {
            return WiFiStatus::CONNECTED_WEAK;
        }
    }

    void wifi_task(void* param) {
        WiFi.hostname("WatchMan");
        WiFi.mode(WIFI_STA);
        constexpr uint64_t CHECK_INTERVAL_MS = 10000; // 10 seconds
        while (true) {
            apps::settings::Settings settings = apps::settings::get_settings();
            if (!settings.wifi_enabled) {
                WiFi.disconnect(true, true); // Disconnect and erase AP
                WiFi.mode(WIFI_OFF); // Turn off WiFi to save power
                vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
                continue;
            }
            if (WiFi.getMode() != WIFI_STA) {
                WiFi.mode(WIFI_STA);
            }
            wl_status_t status = WiFi.status();
            if (status != WL_CONNECTED && status != WL_IDLE_STATUS) {
                WiFi.begin(settings.wifi_ssid, settings.wifi_password);
            }
            vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
        }
    }

    void init() {
        xTaskCreate(wifi_task, "WiFiTask", 4096, nullptr, 1, nullptr);
    }
}