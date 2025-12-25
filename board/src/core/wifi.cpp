#include <WiFi.h>

#include "core/wifi.hpp"
#include "private.hpp"

namespace wifi {
    WiFiStatus get_status() {
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
        constexpr uint64_t NO_SSID_FOUND_DELAY_MS = 60000; // 1 minute
        constexpr uint64_t CHECK_INTERVAL_MS = 10000; // 10 seconds
        // If not connected, or connecting, look for SSID, if found connect
        // If no SSID found, shut down WiFi to save power, retry in a while
        // If connected, do nothing and wait
        while (true) {
            wl_status_t status = WiFi.status();
            if (status != WL_CONNECTED && status != WL_IDLE_STATUS) {
                int16_t n = WiFi.scanNetworks();
                bool ssid_found = false;
                for (int16_t i = 0; i < n; ++i) {
                    String found_ssid = WiFi.SSID(i);
                    if (found_ssid == WIFI_SSID) {
                        ssid_found = true;
                        break;
                    }
                }
                if (ssid_found) {
                    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                    vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
                } else {
                    WiFi.disconnect(true, true); // Disconnect and erase AP
                    WiFi.mode(WIFI_OFF); // Turn off WiFi to save power
                    vTaskDelay(pdMS_TO_TICKS(NO_SSID_FOUND_DELAY_MS));
                    WiFi.mode(WIFI_STA); // Re-enable WiFi for next scan
                    continue;
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
            }
        }
    }

    void init() {
        xTaskCreate(wifi_task, "WiFiTask", 4096, nullptr, 1, nullptr);
    }
}