#pragma once

namespace logger {
    void init();
    void info(const char* fmt, ...);
    void warning(const char* fmt, ...);
    void error(const char* fmt, ...);
}