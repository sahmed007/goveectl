#include "govee.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace govee {

namespace {
constexpr char cache_filename[] = "devices.json";
} // namespace

void save_devices_cache(const std::vector<Device>& devices) {
    auto entries = nlohmann::json::array();
    for (const auto& device : devices) {
        auto entry = nlohmann::json::object(
            {{"device", device.id}, {"sku", device.sku}});
        entries.push_back(entry);
    }

    std::ofstream cache_file(cache_filename);
    if (!cache_file.is_open()) {
        throw std::runtime_error(
            "Error: Could not create or open device cache file");
    }

    cache_file << entries.dump(2);
    cache_file.close();

    if (cache_file.fail()) {
        throw std::runtime_error("Error: Could not write to device cache file");
    }
}

std::vector<Device> load_devices_cache() {
    std::ifstream cache_file(cache_filename);
    if (!cache_file.is_open()) {
        throw std::runtime_error("Error: Could not load device cache file. Try "
                                 "running 'goveectl devices' first");
    }

    auto entries = nlohmann::json::parse(cache_file);
    if (!entries.is_array()) {
        throw std::runtime_error("Expected a device array");
    }

    std::vector<Device> devices{};
    for (const auto& entry : entries) {
        Device device{.sku = entry.at("sku").get<std::string>(),
                      .id = entry.at("device").get<std::string>()};
        devices.push_back(device);
    }

    return devices;
}

} // namespace govee
