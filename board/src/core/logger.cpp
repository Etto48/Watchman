#include <stdarg.h>
#include <Arduino.h>

#include "core/logger.hpp"
#include "core/timekeeper.hpp"
#include "constants.hpp"

namespace logger {
    enum class LogLevel
    {
        INFO,
        WARNING,
        ERROR
    };

    const char* LogLevel_to_cstr(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARNING:
                return "WARNING";
            case LogLevel::ERROR:
                return "ERROR";
            default:
                return "UNKNOWN";
        }
    }

    const char* LogLevel_to_color_code(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:
                return "\033[0;32m"; // Green
            case LogLevel::WARNING:
                return "\033[0;33m"; // Yellow
            case LogLevel::ERROR:
                return "\033[0;31m"; // Red
            default:
                return "";
        }
    }

    void log_message(LogLevel level, const char* fmt, va_list args) {
        char buffer[256];
        auto us = timekeeper::now_us();
        auto seconds = us / 1000000;
        auto micros = us % 1000000;
        auto color = LogLevel_to_color_code(level);
        auto task_name = pcTaskGetName(nullptr);
        Serial.printf("[%lld.%06lld] (%s) [%s%s\033[0m] ", seconds, micros, task_name, color, LogLevel_to_cstr(level));
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
        Serial.print(buffer);
        Serial.println();
    }

    void init() {
        Serial.begin(MONITOR_SPEED);
    }

    void info(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_message(LogLevel::INFO, fmt, args);
        va_end(args);
    }

    void warning(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_message(LogLevel::WARNING, fmt, args);
        va_end(args);
    }

    void error(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_message(LogLevel::ERROR, fmt, args);
        va_end(args);
    }
}