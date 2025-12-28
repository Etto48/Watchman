#include "apps/map.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "images/earth_128x64.hpp"
#include "images/earth_256x128.hpp"
#include "images/earth_512x256.hpp"
#include "constants.hpp"

namespace apps::map {
    enum class ZoomLevel {
        WORLD_FAST = 0,
        WORLD_SLOW = 1,
        CONTINENT_FAST = 2,
        CONTINENT_SLOW = 3,
        COUNTRY_FAST = 4,
        COUNTRY_SLOW = 5,
        INFO = 6
    };

    struct LatLonState {
        double latitude;
        double longitude;
        ZoomLevel zoom_level;
    };

    LatLonState state = {
        .latitude = 50.0,
        .longitude = 0.0,
        .zoom_level = ZoomLevel::WORLD_FAST,
    };

    struct MapProperties {
        const uint8_t* image_data;
        size_t width;
        size_t height;
    };

    constexpr double step_sizes[] = {
        20.0, // WORLD_FAST
        10.0,  // WORLD_SLOW
        5.0,  // CONTINENT_FAST
        2.5, // CONTINENT_SLOW
        1.0,  // COUNTRY_FAST
        0.5,   // COUNTRY_SLOW
        0.0    // INFO
    };

    constexpr MapProperties map_properties[] = {
        { .image_data = images::earth_128x64, .width = images::earth_128x64_width, .height = images::earth_128x64_height }, // WORLD_*
        { .image_data = images::earth_128x64, .width = images::earth_128x64_width, .height = images::earth_128x64_height }, // WORLD_*
        { .image_data = images::earth_256x128, .width = images::earth_256x128_width, .height = images::earth_256x128_height }, // CONTINENT_*
        { .image_data = images::earth_256x128, .width = images::earth_256x128_width, .height = images::earth_256x128_height }, // CONTINENT_*
        { .image_data = images::earth_512x256, .width = images::earth_512x256_width, .height = images::earth_512x256_height },  // COUNTRY_*
        { .image_data = images::earth_512x256, .width = images::earth_512x256_width, .height = images::earth_512x256_height },  // COUNTRY_*
        { .image_data = nullptr, .width = 0, .height = 0 } // INFO
    };

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        auto step_size = step_sizes[static_cast<size_t>(state.zoom_level)];
        switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::UP:
                        if (state.zoom_level == ZoomLevel::INFO) break;
                        sound::play_navigation_tone();
                        state.latitude += step_size;
                        if (state.latitude > 90.0) state.latitude = 90.0;
                        menu::set_dirty();
                        break;
                    case events::Button::DOWN:
                        if (state.zoom_level == ZoomLevel::INFO) break;
                        sound::play_navigation_tone();
                        state.latitude -= step_size;
                        if (state.latitude < -90.0) state.latitude = -90.0;
                        menu::set_dirty();
                        break;
                    case events::Button::LEFT:
                        if (state.zoom_level == ZoomLevel::INFO) break;
                        sound::play_navigation_tone();
                        state.longitude -= step_size;
                        if (state.longitude < -180.0) state.longitude += 360.0;
                        menu::set_dirty();
                        break;
                    case events::Button::RIGHT:
                        if (state.zoom_level == ZoomLevel::INFO) break;
                        sound::play_navigation_tone();
                        state.longitude += step_size;
                        if (state.longitude > 180.0) state.longitude -= 360.0;
                        menu::set_dirty();
                        break;
                    case events::Button::A:
                        if (state.zoom_level != ZoomLevel::INFO) {
                            sound::play_confirm_tone();
                            state.zoom_level = static_cast<ZoomLevel>(static_cast<size_t>(state.zoom_level) + 1);
                            menu::set_dirty();
                        }
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        if (state.zoom_level != ZoomLevel::WORLD_FAST) {
                            state.zoom_level = static_cast<ZoomLevel>(static_cast<size_t>(state.zoom_level) - 1);
                            menu::set_dirty();
                        } else {
                            menu::current_app = menu::App::NONE;
                            menu::set_dirty();
                        }
                        break;
                }
                break;
            case events::EventType::NONE:
                menu::upkeep(display);
                break;
            default:
                break;
        }
    }

    struct MapCoords {
        size_t x;
        size_t y;
    };

    MapCoords latlon_to_xy(double latitude, double longitude, ZoomLevel zoom_level) {
        auto props = map_properties[static_cast<size_t>(zoom_level)];
        // Equirectangular projection
        size_t x = static_cast<size_t>(((longitude + 180.0) / 360.0) * props.width) % props.width;
        size_t y = static_cast<size_t>(((90.0 - latitude) / 180.0) * props.height);
        if (y >= props.height) y = props.height - 1;
        return { x, y };
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        if (state.zoom_level == ZoomLevel::INFO) {
            menu::draw_generic_titlebar(display, "Map");
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 16);
            display.println("Latitude:");
            display.printf("  %.4f\n", state.latitude);
            display.println("Longitude:");
            display.printf("  %.4f\n", state.longitude);
        } else {
            auto props = map_properties[static_cast<size_t>(state.zoom_level)];
            auto cursor = latlon_to_xy(state.latitude, state.longitude, state.zoom_level);
            auto top_left_corner = MapCoords {
                .x = (cursor.x + props.width - static_cast<size_t>(SCREEN_WIDTH) / 2) % props.width,
                .y = (cursor.y < static_cast<size_t>(SCREEN_HEIGHT) / 2) ? 0 : (cursor.y > props.height - static_cast<size_t>(SCREEN_HEIGHT) / 2) ? (props.height - static_cast<size_t>(SCREEN_HEIGHT)) : (cursor.y - static_cast<size_t>(SCREEN_HEIGHT) / 2)
            };
            for (size_t x = 0; x < static_cast<size_t>(SCREEN_WIDTH); x++) {
                for (size_t y = 0; y < static_cast<size_t>(SCREEN_HEIGHT); y++) {
                    size_t map_x = (top_left_corner.x + x) % props.width;
                    size_t map_y = top_left_corner.y + y;
                    bool pixel = props.image_data[map_y * (props.width / 8) + map_x / 8] & (0x80 >> (map_x % 8));
                    if (map_x == cursor.x || map_y == cursor.y) {
                        pixel = !pixel; // Invert color for crosshair
                    }
                    display.drawPixel(x, y, pixel ? SSD1306_WHITE : SSD1306_BLACK);
                }
            }
        }
        display.display();
    }
}