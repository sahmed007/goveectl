#include "CLI11.hpp"
#include "cli.hpp"
#include "govee.hpp"
#include "terminal.hpp"
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
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

void print_devices(const std::vector<govee::Device>& devices) {
    if (devices.empty()) {
        std::cout << "No devices found.\n";
        return;
    }

    std::size_t name_width = 4;
    std::size_t model_width = 5;
    for (const auto& device : devices) {
        const auto name = device.name.empty() ? "(unnamed)" : device.name;
        name_width = std::max(name_width, name.size());
        model_width = std::max(model_width, device.sku.size());
    }
    name_width += 2;
    model_width += 2;

    std::ostringstream header;
    header << std::left << std::setw(static_cast<int>(name_width)) << "NAME"
           << std::setw(static_cast<int>(model_width)) << "MODEL"
           << "DEVICE ID";
    std::cout << terminal::style(header.str(), "1") << '\n';

    std::ostringstream rows;
    rows << std::left;
    for (const auto& device : devices) {
        rows << std::setw(static_cast<int>(name_width))
             << (device.name.empty() ? "(unnamed)" : device.name)
             << std::setw(static_cast<int>(model_width)) << device.sku
             << device.id << '\n';
    }
    std::cout << rows.str();
}

void show_devices(const char* api_key) {
    const auto devices = govee::list_devices(api_key);
    govee::save_devices_cache(devices);
    print_devices(devices);
}

void change_power(const std::string& device_selector, bool enabled,
                  const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_power(device, enabled, api_key);
    std::cout << terminal::success("Success: ") << "Power "
              << (enabled ? "on" : "off") << " request succeeded for "
              << (device.name.empty() ? device.id : device.name) << '\n';
}

void change_brightness(const std::string& device_selector, int value,
                       const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_brightness(device, value, api_key);
    std::cout << terminal::success("Success: ") << "Changed brightness of "
              << device.name << " to " << value << '\n';
}

void change_color(const std::string& device_selector, int red, int green,
                  int blue, const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_color(device, red, green, blue, api_key);
    std::cout << terminal::success("Success: ") << "Changed color to RGB("
              << red << ", " << green << ", " << blue << ")\n";
}
} // namespace

int main(int argc, char** argv) {
    Options options;
    CLI::App app{"goveectl — control Govee devices from your terminal"};

    configure_cli(app, options);
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
        std::ostringstream output;
        std::ostringstream errors;
        const int status = app.exit(error, output, errors);
        std::cout << output.str();
        std::cerr << terminal::error(errors.str());
        return status;
    }

    const char* api_key = std::getenv("GOVEE_API_KEY");
    if (api_key == nullptr || api_key[0] == '\0') {
        std::cerr << terminal::error("GOVEE_API_KEY is not set") << '\n';
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
            change_color(options.device, options.red, options.green,
                         options.blue, api_key);
            break;
        case Command::none:
            std::cerr << terminal::error("No command selected") << '\n';
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << terminal::error(error.what()) << '\n';
        return 1;
    }

    return 0;
}
