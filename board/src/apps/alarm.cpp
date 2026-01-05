#include "alarm.hpp"
#include "core/timekeeper.hpp"
#include "core/events.hpp"
#include "core/menu.hpp"
#include "core/sound.hpp"
#include "constants.hpp"

namespace apps::alarm {
    SemaphoreHandle_t alarm_mutex = nullptr;
    // Time of day in seconds, 0 is midnight
    RTC_DATA_ATTR uint64_t alarm_time_of_day = 0;
    RTC_DATA_ATTR bool alarm_enabled = false;
    RTC_DATA_ATTR time_t last_alarm_snoozed = 0;
    bool alarm_is_playing = false;
    bool rtc_available = false;

    enum class SelectedField {
        HOURS,
        MINUTES,
        SECONDS,
        ENABLE,
    };

    SelectedField selected_field = SelectedField::HOURS;

    constexpr uint64_t SECOND_TICKS = 1;
    constexpr uint64_t MINUTE_TICKS = SECOND_TICKS * 60;
    constexpr uint64_t HOUR_TICKS = MINUTE_TICKS * 60;
    constexpr uint64_t DAY_TICKS = HOUR_TICKS * 24;

    // Returns 0 if alarm is disabled or RTC not synced
    TimestampAndTriggered get_alarm_timestamp() {
        tm timeinfo = {0};
        getLocalTime(&timeinfo, 0);
        time_t now = mktime(&timeinfo);
        if (timeinfo.tm_year + 1900 < 2025) {
            return {0, false}; // RTC not synced
        }
        if (xSemaphoreTake(alarm_mutex, portMAX_DELAY) == pdTRUE) {
            auto triggered = alarm_is_playing;
            if (!alarm_enabled) {
                xSemaphoreGive(alarm_mutex);
                return {0, false};
            } else {
                timeinfo.tm_hour = alarm_time_of_day / HOUR_TICKS;
                timeinfo.tm_min = (alarm_time_of_day % HOUR_TICKS) / MINUTE_TICKS;
                timeinfo.tm_sec = (alarm_time_of_day % MINUTE_TICKS) / SECOND_TICKS;
                time_t alarm_today = mktime(&timeinfo);
                if (last_alarm_snoozed >= alarm_today) {
                    // Alarm time today has already passed, schedule for tomorrow
                    alarm_today += 24 * 3600;
                }
                xSemaphoreGive(alarm_mutex);
                return {alarm_today, triggered};
            }
        } else {
            return {0, false};
        }
    }

    void snooze_alarm() {
        tm timeinfo = {0};
        getLocalTime(&timeinfo, 0);
        time_t now = mktime(&timeinfo);
        xSemaphoreTake(alarm_mutex, portMAX_DELAY);
        last_alarm_snoozed = now;
        alarm_is_playing = false;
        xSemaphoreGive(alarm_mutex);
        sound::stop_async_interruptible_melody();
    }

    void change_selected_value(bool up) {
        xSemaphoreTake(alarm_mutex, portMAX_DELAY);
        const int64_t delta = up ? 1 : -1;
        switch (selected_field) {
            case SelectedField::HOURS: {
                alarm_time_of_day += DAY_TICKS + delta * HOUR_TICKS;
                alarm_time_of_day %= DAY_TICKS;
                break;
            }
            case SelectedField::MINUTES: {
                alarm_time_of_day += DAY_TICKS + delta * MINUTE_TICKS;
                alarm_time_of_day %= DAY_TICKS;
                break;
            }
            case SelectedField::SECONDS: {
                alarm_time_of_day += DAY_TICKS + delta * SECOND_TICKS;
                alarm_time_of_day %= DAY_TICKS;
                break;
            }
            case SelectedField::ENABLE: {
                alarm_enabled = !alarm_enabled;
                break;
            }
        }
        last_alarm_snoozed = timekeeper::rtc_s(); // Reset snooze to now to avoid immediate retrigger
        xSemaphoreGive(alarm_mutex);
    }

    void change_selected_field(bool next) {
        switch (selected_field) {
            case SelectedField::HOURS:
                selected_field = next ? SelectedField::MINUTES : SelectedField::ENABLE;
                break;
            case SelectedField::MINUTES:
                selected_field = next ? SelectedField::SECONDS : SelectedField::HOURS;
                break;
            case SelectedField::SECONDS:
                selected_field = next ? SelectedField::ENABLE : SelectedField::MINUTES;
                break;
            case SelectedField::ENABLE:
                selected_field = next ? SelectedField::HOURS : SelectedField::SECONDS;
                break;
        }
    }

    void draw(Adafruit_SSD1306& display) {
        display.clearDisplay();
        menu::draw_generic_titlebar(display, "Alarm");
        xSemaphoreTake(alarm_mutex, portMAX_DELAY);
        auto seconds = (alarm_time_of_day / SECOND_TICKS) % 60;
        auto minutes = (alarm_time_of_day / MINUTE_TICKS) % 60;
        auto hours = (alarm_time_of_day / HOUR_TICKS) % 24;
        auto local_alarm_enabled = apps::alarm::alarm_enabled;
        auto local_last_alarm_snoozed = apps::alarm::last_alarm_snoozed;
        auto local_alarm_is_playing = apps::alarm::alarm_is_playing;
        xSemaphoreGive(alarm_mutex);
        if (local_alarm_is_playing) {
            display.fillRect(0, 24, SCREEN_WIDTH, 16, SSD1306_WHITE);
            display.setTextColor(SSD1306_INVERSE);
            display.setTextSize(2);
            display.setCursor(30, 25);
            display.println("ALARM!");
            display.setTextSize(1);
            display.setCursor(10, 50);
            display.println("Press A to snooze");
            display.display();
            return;
        }
        display.setTextSize(2);
        switch (selected_field) {
            case SelectedField::HOURS:
            display.fillRect(19, 19, 24, 16, SSD1306_WHITE);
            break;
            case SelectedField::MINUTES:
            display.fillRect(55, 19, 24, 16, SSD1306_WHITE);
            break;
            case SelectedField::SECONDS:
            display.fillRect(91, 19, 24, 16, SSD1306_WHITE);
            break;
            case SelectedField::ENABLE:
            break;
        }
        display.setCursor(20, 20);
        display.setTextColor(SSD1306_INVERSE);
        display.printf("%02llu:%02llu:%02llu", hours, minutes, seconds);
        display.setTextSize(1);
        if (selected_field == SelectedField::ENABLE) {
            display.fillRect(0, 50, SCREEN_WIDTH, 16, SSD1306_WHITE);   
        }
        if (!rtc_available) {
            display.setCursor(10, 51);
            display.printf("sync RTC to enable");
        } else {
            display.setCursor(30, 51);
            display.printf("Alarm: %s", local_alarm_enabled ? "ON" : "OFF");
        }
        display.display();
    }

    void app(Adafruit_SSD1306& display) {
        events::Event ev = events::get_next_event();
        xSemaphoreTake(alarm_mutex, portMAX_DELAY);
        bool local_alarm_is_playing = alarm_is_playing;
        xSemaphoreGive(alarm_mutex);
        if (local_alarm_is_playing) {
            if (ev.type == events::EventType::BUTTON_PRESS &&
                ev.button_press_event.button == events::Button::A) {
                snooze_alarm();
                menu::set_dirty();
            } else if (ev.type == events::EventType::NONE) {
                menu::upkeep(display);
            }
        } else {
            switch (ev.type) {
            case events::EventType::BUTTON_PRESS:
                switch (ev.button_press_event.button) {
                    case events::Button::UP:
                        sound::play_navigation_tone();
                        change_selected_value(true);
                        menu::set_dirty();
                        break;
                    case events::Button::DOWN:
                        sound::play_navigation_tone();
                        change_selected_value(false);
                        menu::set_dirty();
                        break;
                    case events::Button::LEFT:
                        sound::play_navigation_tone();    
                        change_selected_field(false);
                        menu::set_dirty();
                        break;
                    case events::Button::RIGHT:
                        sound::play_navigation_tone();
                        change_selected_field(true);
                        menu::set_dirty();
                        break;
                    case events::Button::A:
                        if (selected_field == SelectedField::ENABLE) {
                            sound::play_navigation_tone();
                            change_selected_value(true);
                            menu::set_dirty();
                        }
                        break;
                    case events::Button::B:
                        sound::play_cancel_tone();
                        menu::current_app = menu::App::NONE;
                        menu::set_dirty();
                        break;
                    default:
                        break;
                }
                
                break;
            case events::EventType::NONE:
                {
                    bool new_rtc_available = timekeeper::rtc_s() != 0;
                    if (new_rtc_available != rtc_available) {
                        rtc_available = new_rtc_available;
                        menu::set_dirty();
                    }
                }
                menu::upkeep(display);
                break;
            }
        }
    }

    sound::Note alarm_tone[] = {
        { sound::NoteFrequency::NOTE_A4, 200 },
        { sound::NoteFrequency::NOTE_REST, 100 },
        { sound::NoteFrequency::NOTE_A4, 200 },
        { sound::NoteFrequency::NOTE_REST, 100 },
        { sound::NoteFrequency::NOTE_A4, 100 },
        { sound::NoteFrequency::NOTE_C5, 100 },
        { sound::NoteFrequency::NOTE_E5, 100 },
        { sound::NoteFrequency::NOTE_REST, 300 },
    };

    void alarm_task(void* params) {
        tm timeinfo = {0};
        while (true) {
            getLocalTime(&timeinfo, 0);
            if (timeinfo.tm_year + 1900 >= 2025) {
                time_t now = mktime(&timeinfo);
                TimestampAndTriggered alarm_info = get_alarm_timestamp();
                if (alarm_info.timestamp != 0 && now >= alarm_info.timestamp && !alarm_info.triggered) {
                    sound::async_play_interruptible_melody(alarm_tone, sizeof(alarm_tone)/sizeof(alarm_tone[0]), true);
                    xSemaphoreTake(alarm_mutex, portMAX_DELAY);
                    alarm_is_playing = true;
                    xSemaphoreGive(alarm_mutex);
                    menu::set_dirty();
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    void init() {
        alarm_mutex = xSemaphoreCreateMutex();
        xTaskCreate(alarm_task, "AlarmTask", 2048, nullptr, 1, nullptr);
    }
}