#pragma once

#include "govee.hpp"
#include <vector>

namespace display {
struct DeviceRow {
    govee::Device device;
    govee::DeviceState state;
    bool failed = false;
};

void print_devices(const std::vector<DeviceRow>& devices);

void print_status(const govee::Device& device, const govee::DeviceState& state);
} // namespace display
