#include "CLI11.hpp"
#include "cli.hpp"
#include "govee.hpp"
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
govee::Device find_device(const std::string& device_selector) {
    const auto devices = govee::load_devices_cache();
    auto it = std::ranges::find(devices, device_selector, &govee::Device::id);
    if (it == devices.end() && !device_selector.empty()) {
        for (auto candidate = devices.begin(); candidate != devices.end();
             ++candidate) {
            if (candidate->name != device_selector) {
                continue;
            }
            if (it != devices.end()) {
                throw std::runtime_error(
                    "Multiple devices named '" + device_selector +
                    "'. Use a device ID from 'goveectl devices'.");
            }
            it = candidate;
        }
    }

    if (it == devices.end()) {
        throw std::runtime_error(
            "Device name or ID not found in cache (names are case-sensitive). "
            "Run 'goveectl devices' to refresh.");
    }

    return *it;
}

void show_devices(const char* api_key) {
    std::vector<govee::Device> devices = govee::list_devices(api_key);
    govee::save_devices_cache(devices);
    for (const auto& device : devices) {
        std::cout << "Name: "
                  << (device.name.empty() ? "(unnamed)" : device.name)
                  << " | Device SKU: " << device.sku << " and "
                  << "Device ID: " << device.id << '\n';
    }
}

void change_power(const std::string& device_selector, bool enabled,
                  const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_power(device, enabled, api_key);
    std::cout << "Power " << (enabled ? "on" : "off")
              << " request succeeded for "
              << (device.name.empty() ? device.id : device.name) << '\n';
}

void change_brightness(const std::string& device_selector, int value,
                       const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_brightness(device, value, api_key);
    std::cout << "Changed brightness off " << device.name << " to " << value
              << '\n';
}
} // namespace

int main(int argc, char** argv) {
    Options options;
    CLI::App app{"Govee Cloud CLI"};

    configure_cli(app, options);
    CLI11_PARSE(app, argc, argv);

    const char* api_key = std::getenv("GOVEE_API_KEY");
    if (api_key == nullptr || api_key[0] == '\0') {
        std::cerr << "GOVEE_API_KEY is not set\n";
        return 1;
    }

    try {
        switch (options.command) {
        case Command::devices:
            show_devices(api_key);
            break;
        case Command::power_on:
            change_power(options.device, true, api_key);
            break;
        case Command::power_off:
            change_power(options.device, false, api_key);
            break;
        case Command::brightness:
            change_brightness(options.device, options.brightness, api_key);
            break;
        case Command::color:
            std::cout << "Setting color to RGB(" << options.red << ", "
                      << options.green << ", " << options.blue << ")\n";
            break;
        case Command::none:
            std::cerr << "No command selected" << '\n';
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
