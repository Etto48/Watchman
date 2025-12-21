#pragma once

namespace wifi {
    enum class WiFiStatus : unsigned int {
        DISCONNECTED = 0,
        CONNECTED_WEAK = 1,
        CONNECTED_AVERAGE = 2,
        CONNECTED_STRONG = 3,
    };

    // Get the current WiFi status and approximate signal strength
    WiFiStatus get_status();

    // Start a WiFi connection, non-blocking
    void connect();
}