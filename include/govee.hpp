#pragma once

#include <optional>
#include <string>
#include <vector>

namespace govee {
struct Device {
    std::string sku;
    std::string id;
    std::string name;
};

struct DeviceState {
    std::optional<bool> online;
    std::optional<bool> power;
    std::optional<int> brightness;
    std::optional<int> color_rgb;
    std::optional<int> temperature_kelvin;
};

std::vector<Device> list_devices(const std::string& api_key);

DeviceState get_device_state(const Device& device, const std::string& api_key);

void set_power(const Device& device, bool enabled, const std::string& api_key);

void set_brightness(const Device& device, int value, const std::string& api_key);

void set_color(const Device& device, int red, int green, int blue, const std::string& api_key);

void set_temperature(const Device& device, int kelvin, const std::string& api_key);

void save_devices_cache(const std::vector<Device>& devices);

std::vector<Device> load_devices_cache();

} // namespace govee
