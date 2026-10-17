#include "govee.hpp"
#include "device_json.hpp" // IWYU pragma: keep
#include "uuid.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace govee {

namespace {

nlohmann::json parse_response(const cpr::Response& response) {
    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Network error: " + response.error.message);
    }

    if (response.status_code != 200) {
        throw std::runtime_error("HTTP error: " +
                                 std::to_string(response.status_code));
    }

    const auto body = nlohmann::json::parse(response.text, nullptr, false);
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
        }
    }

    // Keep offline readings as last-known values; callers must label them as
    // such.
    return state;
}

void send_control(const Device& device, const nlohmann::json& capability,
                  const std::string& api_key) {
    nlohmann::json payload = {
        {"sku", device.sku}, {"device", device.id}, {"capability", capability}};

    const auto id = uuids::uuid_system_generator{}();
    nlohmann::json request_body = {{"requestId", uuids::to_string(id)},
                                   {"payload", payload}};

    cpr::Response response = cpr::Post(
        cpr::Url{"https://openapi.api.govee.com/router/api/v1/device/control"},
        cpr::Header{{"Govee-API-Key", api_key},
                    {"Content-Type", "application/json"}},
        cpr::Body{request_body.dump()}, cpr::Timeout(10000));

    parse_response(response);
}

} // namespace

std::vector<Device> list_devices(const std::string& api_key) {
    cpr::Response response = cpr::Get(
        cpr::Url{"https://openapi.api.govee.com/router/api/v1/user/devices"},
        cpr::Header{{"Govee-API-Key", api_key}, {"Accept", "application/json"}},
        cpr::Timeout(10000));

    const auto body = parse_response(response);

    const auto& entries = body.at("data");
    if (!entries.is_array()) {
        throw std::runtime_error("Expected a device array");
    }

    return entries.get<std::vector<Device>>();
}

DeviceState get_device_state(const Device& device, const std::string& api_key) {
    nlohmann::json payload = {{"sku", device.sku}, {"device", device.id}};
    const auto id = uuids::uuid_system_generator{}();
    nlohmann::json request_body = {{"requestId", uuids::to_string(id)},
                                   {"payload", payload}};

    cpr::Response response = cpr::Post(
        cpr::Url{"https://openapi.api.govee.com/router/api/v1/device/state"},
        cpr::Header{{"Govee-API-Key", api_key},
                    {"Content-Type", "application/json"}},
        cpr::Body{request_body.dump()}, cpr::Timeout(10000));

    return parse_device_state(parse_response(response));
}

void set_power(const Device& device, bool enabled, const std::string& api_key) {
    nlohmann::json capability = {{"type", "devices.capabilities.on_off"},
                                 {"instance", "powerSwitch"},
                                 {"value", enabled ? 1 : 0}};
    send_control(device, capability, api_key);
}

void set_brightness(const Device& device, int value,
                    const std::string& api_key) {
    nlohmann::json capability = {{"type", "devices.capabilities.range"},
                                 {"instance", "brightness"},
                                 {"value", value}};
    send_control(device, capability, api_key);
}

void set_color(const Device& device, int red, int green, int blue,
               const std::string& api_key) {
    int rgb = red * 256 * 256 + green * 256 + blue;
    nlohmann::json capability = {{"type", "devices.capabilities.color_setting"},
                                 {"instance", "colorRgb"},
                                 {"value", rgb}};
    send_control(device, capability, api_key);
}

void set_temperature(const Device& device, int kelvin,
                     const std::string& api_key) {
    if (kelvin <= 0) {
        throw std::invalid_argument("Color temperature must be positive Kelvin");
    }
    const nlohmann::json capability = {
        {"type", "devices.capabilities.color_setting"},
        {"instance", "colorTemperatureK"},
        {"value", kelvin}};
    send_control(device, capability, api_key);
}

} // namespace govee
