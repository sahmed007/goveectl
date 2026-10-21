#pragma once

#include <string>

namespace commands {
bool show_devices(const char* api_key);

void show_status(const std::string& device_selector, const char* api_key);

void change_power(const std::string& device_selector, bool enabled, const char* api_key);

void change_brightness(const std::string& device_selector, int value, const char* api_key);

void change_color(const std::string& device_selector, int red, int green, int blue,
                  const char* api_key);

void change_temperature(const std::string& device_selector, int kelvin, const char* api_key);
} // namespace commands
