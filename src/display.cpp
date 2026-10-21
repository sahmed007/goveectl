#include "display.hpp"
#include "terminal.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace display {
namespace {
std::string online_text(const std::optional<bool>& online) {
    if (!online.has_value()) {
        return "Unknown";
    }
    return *online ? "Online" : "Offline";
}

std::string power_text(const std::optional<bool>& power) {
    if (!power.has_value()) {
        return "Unknown";
    }
    return *power ? "On" : "Off";
}

} // namespace

void print_devices(const std::vector<DeviceRow>& devices) {
    if (devices.empty()) {
        std::cout << "No devices found.\n";
        return;
    }

    std::size_t name_width = 4;
    std::size_t model_width = 5;
    for (const auto& row : devices) {
        const auto& device = row.device;
        const auto name = device.name.empty() ? "(unnamed)" : device.name;
        name_width = std::max(name_width, name.size());
        model_width = std::max(model_width, device.sku.size());
    }
    name_width += 2;
    model_width += 2;

    std::ostringstream header;
    header << std::left << std::setw(static_cast<int>(name_width)) << "NAME"
           << std::setw(static_cast<int>(model_width)) << "MODEL" << std::setw(10) << "STATUS"
           << std::setw(9) << "POWER"
           << "DEVICE ID";
    std::cout << terminal::style(header.str(), "1") << '\n';

    std::ostringstream rows;
    rows << std::left;
    for (const auto& row : devices) {
        const auto& device = row.device;
        const auto status = row.failed ? "Error" : online_text(row.state.online);
        const auto power = row.failed                 ? "Error"
                           : row.state.online == true ? power_text(row.state.power)
                                                      : "Unknown";
        rows << std::setw(static_cast<int>(name_width))
             << (device.name.empty() ? "(unnamed)" : device.name)
             << std::setw(static_cast<int>(model_width)) << device.sku << std::setw(10) << status
             << std::setw(9) << power << device.id << '\n';
    }
    std::cout << rows.str();
}

void print_status(const govee::Device& device, const govee::DeviceState& state) {
    std::cout << "Name: " << (device.name.empty() ? "(unnamed)" : device.name)
              << "\nModel: " << device.sku << "\nDevice ID: " << device.id
              << "\nStatus: " << online_text(state.online) << '\n';
    const std::string qualifier = state.online == false       ? " (last-known)"
                                  : !state.online.has_value() ? " (unverified)"
                                                              : "";
    std::cout << "Power" << qualifier << ": " << power_text(state.power) << '\n'
              << "Brightness" << qualifier << ": "
              << (state.brightness ? std::to_string(*state.brightness) + "%" : "Unknown") << '\n';
    std::ostringstream color;
    if (state.color_rgb) {
        color << '#' << std::uppercase << std::hex << std::setfill('0') << std::setw(6)
              << *state.color_rgb;
    } else {
        color << "Unknown";
    }
    std::cout << "Color" << qualifier << ": " << color.str() << '\n';
    std::cout << "Temperature" << qualifier << ": "
              << (state.temperature_kelvin ? std::to_string(*state.temperature_kelvin) + " K"
                                           : "Unknown")
              << '\n';
}

} // namespace display
