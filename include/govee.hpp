#pragma once

#include <string>
#include <vector>

namespace govee {
struct Device {
    std::string sku;
    std::string id;
    std::string name;
};

std::vector<Device> list_devices(const std::string& api_key);

void set_power(const Device& device, bool enabled, const std::string& api_key);

void set_brightness(const Device& device, int value,
                    const std::string& api_key);

void save_devices_cache(const std::vector<Device>& devices);

std::vector<Device> load_devices_cache();

} // namespace govee
