#pragma once

#include "govee.hpp"
#include <nlohmann/json.hpp>

// Internal pure helpers shared by the application and offline tests.
namespace govee::detail {
nlohmann::json parse_response(long status_code, const std::string& text);
DeviceState parse_device_state(const nlohmann::json& body);
Device find_device(const std::vector<Device>& devices,
                   const std::string& device_selector);
} // namespace govee::detail
