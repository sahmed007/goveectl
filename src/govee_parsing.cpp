#include "govee_internal.hpp"
#include <limits>
#include <stdexcept>

namespace govee::detail {
nlohmann::json parse_response(long status_code, const std::string& text) {
    if (status_code != 200) {
        throw std::runtime_error("HTTP error: " + std::to_string(status_code));
    }

    const auto body = nlohmann::json::parse(text, nullptr, false);
    if (body.is_discarded()) {
        throw std::runtime_error("Govee API response was not valid JSON");
    }

    if (!body.is_object() || !body.contains("code") ||
        !body.at("code").is_number_integer()) {
        throw std::runtime_error("Govee API response is missing a valid code");
    }

    if (body.at("code") != 200) {
        std::string message =
            "Govee API failed (code " + body.at("code").dump() + ")";
        if (body.contains("message") && body.at("message").is_string()) {
            message += ": " + body.at("message").get<std::string>();
        } else if (body.contains("msg") && body.at("msg").is_string()) {
            message += ": " + body.at("msg").get<std::string>();
        }
        throw std::runtime_error(message);
    }

    return body;
}

DeviceState parse_device_state(const nlohmann::json& body) {
    const auto& capabilities = body.at("payload").at("capabilities");
    if (!capabilities.is_array()) {
        throw std::runtime_error("Expected a device state capabilities array");
    }

    DeviceState state;
    for (const auto& capability : capabilities) {
        if (!capability.is_object() || !capability.contains("state") ||
            !capability.at("state").is_object() ||
            !capability.at("state").contains("value")) {
            continue;
        }
        const auto& value = capability.at("state").at("value");
        const auto type = capability.value("type", std::string{});
        const auto instance = capability.value("instance", std::string{});

        if (type == "devices.capabilities.online" && instance == "online" &&
            value.is_boolean()) {
            state.online = value.get<bool>();
        } else if (type == "devices.capabilities.on_off" &&
                   instance == "powerSwitch" && value.is_number_integer() &&
                   (value == 0 || value == 1)) {
            state.power = value == 1;
        } else if (type == "devices.capabilities.range" &&
                   instance == "brightness" && value.is_number_integer() &&
                   value >= 0 && value <= 100) {
            state.brightness = value.get<int>();
        } else if (type == "devices.capabilities.color_setting" &&
                   instance == "colorRgb" && value.is_number_integer() &&
                   value >= 0 && value <= 16777215) {
            state.color_rgb = value.get<int>();
        } else if (type == "devices.capabilities.color_setting" &&
                   instance == "colorTemperatureK" &&
                   value.is_number_integer() && value > 0 &&
                   value <= std::numeric_limits<int>::max()) {
            state.temperature_kelvin = value.get<int>();
        }
    }

    // Keep offline readings as last-known values; callers must label them as
    // such.
    return state;
}

} // namespace govee::detail
