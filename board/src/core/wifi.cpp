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

    void connect() {
        WiFi.hostname("WatchMan");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
}