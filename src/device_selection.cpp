#include "govee_internal.hpp"
#include <algorithm>
#include <stdexcept>

namespace govee::detail {
Device find_device(const std::vector<Device>& devices, const std::string& device_selector) {
    auto it = std::ranges::find(devices, device_selector, &govee::Device::id);
    if (it == devices.end() && !device_selector.empty()) {
        for (auto candidate = devices.begin(); candidate != devices.end(); ++candidate) {
            if (candidate->name != device_selector) {
                continue;
            }
            if (it != devices.end()) {
                throw std::runtime_error("Multiple devices named '" + device_selector +
                                         "'. Use a device ID from 'goveectl devices'.");
            }
            it = candidate;
        }
    }

    if (it == devices.end()) {
        throw std::runtime_error("Device name or ID not found in cache (names are case-sensitive). "
                                 "Run 'goveectl devices' to refresh.");
    }

    return *it;
}

} // namespace govee::detail
