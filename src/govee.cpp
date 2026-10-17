#include "govee.hpp"
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
        }
        throw std::runtime_error(message);
    }

    return body;
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

    std::vector<Device> devices{};
    for (const auto& entry : entries) {
        Device device{.sku = entry.at("sku").get<std::string>(),
                      .id = entry.at("device").get<std::string>()};

        devices.push_back(device);
    }

    return devices;
}

void set_power(const Device& device, bool enabled, const std::string& api_key) {
    nlohmann::json capability = {{"type", "devices.capabilities.on_off"},
                                 {"instance", "powerSwitch"},
                                 {"value", enabled ? 1 : 0}};

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

} // namespace govee
