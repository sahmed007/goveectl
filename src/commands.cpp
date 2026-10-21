#include "commands.hpp"
#include "display.hpp"
#include "govee.hpp"
#include "govee_internal.hpp"
#include "terminal.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace commands {
namespace {
govee::Device find_device(const std::string& device_selector) {
    const auto devices = govee::load_devices_cache();
    return govee::detail::find_device(devices, device_selector);
}

} // namespace

bool show_devices(const char* api_key) {
    const auto devices = govee::list_devices(api_key);
    govee::save_devices_cache(devices);
    std::vector<display::DeviceRow> rows;
    bool success = true;
    for (const auto& device : devices) {
        display::DeviceRow row{device, {}, false};
        try {
            row.state = govee::get_device_state(device, api_key);
        } catch (const std::exception& error) {
            row.failed = true;
            success = false;
            std::cerr << terminal::error("State query failed for " + device.id + ": " +
                                         error.what())
                      << '\n';
        }
        rows.push_back(row);
    }
    display::print_devices(rows);
    return success;
}

void show_status(const std::string& device_selector, const char* api_key) {
    const auto device = find_device(device_selector);
    const auto state = govee::get_device_state(device, api_key);
    display::print_status(device, state);
}

void change_power(const std::string& device_selector, bool enabled, const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_power(device, enabled, api_key);
    std::cout << terminal::success("Success: ") << "Power " << (enabled ? "on" : "off")
              << " request succeeded for " << (device.name.empty() ? device.id : device.name)
              << '\n';
}

void change_brightness(const std::string& device_selector, int value, const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_brightness(device, value, api_key);
    std::cout << terminal::success("Success: ") << "Changed brightness of " << device.name << " to "
              << value << '\n';
}

void change_color(const std::string& device_selector, int red, int green, int blue,
                  const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_color(device, red, green, blue, api_key);
    std::cout << terminal::success("Success: ") << "Changed color to RGB(" << red << ", " << green
              << ", " << blue << ")\n";
}

void change_temperature(const std::string& device_selector, int kelvin, const char* api_key) {
    const auto device = find_device(device_selector);
    govee::set_temperature(device, kelvin, api_key);
    std::cout << terminal::success("Success: ") << "Temperature request succeeded for "
              << (device.name.empty() ? device.id : device.name) << ": " << kelvin << " K\n";
}

} // namespace commands
