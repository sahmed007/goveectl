#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace terminal {

inline bool supports_color(FILE* stream) {
    if (std::getenv("NO_COLOR") != nullptr) {
        return false;
    }
    const char* term = std::getenv("TERM");
    if (term != nullptr && std::string_view{term} == "dumb") {
        return false;
    }
#ifdef _WIN32
    const int fd = _fileno(stream);
    if (!_isatty(fd)) {
        return false;
    }
    const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    DWORD mode = 0;
    return GetConsoleMode(handle, &mode) &&
           SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    return isatty(fileno(stream)) != 0;
#endif
}

inline std::string style(std::string_view text, std::string_view code, FILE* stream = stdout) {
    if (text.empty() || !supports_color(stream)) {
        return std::string{text};
    }
    return "\033[" + std::string{code} + "m" + std::string{text} + "\033[0m";
}

inline std::string success(std::string_view text) { return style(text, "32"); }

inline std::string error(std::string_view text) { return style(text, "31", stderr); }

} // namespace terminal
