#include "govee.hpp"
#include "cpr/api.h"
#include "cpr/body.h"
#include "cpr/cprtypes.h"
#include "cpr/error.h"
#include "cpr/response.h"
#include "cpr/timeout.h"
#include "uuid.h"
#include <cpr/cpr.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace govee {

std::vector<Device> list_devices(const std::string& api_key) {
    cpr::Response response = cpr::Get(
        cpr::Url{"https://openapi.api.govee.com/router/api/v1/user/devices"},
        cpr::Header{{"Govee-API-Key", api_key}, {"Accept", "application/json"}},
        cpr::Timeout(10000));

    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Network error: " + response.error.message);
    }

    if (response.status_code != 200) {
        throw std::runtime_error("HTTP error: " +
                                 std::to_string(response.status_code));
    }

    const auto body = nlohmann::json::parse(response.text);
    if (body.at("code").get<int>() != 200) {
        throw std::runtime_error("Govee API reported an error");
    }

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

    if (response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Network error: " + response.error.message);
    }

    if (response.status_code != 200) {
        throw std::runtime_error("HTTP error: " +
                                 std::to_string(response.status_code));
    }

    const auto body = nlohmann::json::parse(response.text, nullptr, false);
    if (body.is_discarded()) {
        throw std::runtime_error("Govee control response was not valid JSON");
    }

    if (!body.is_object() || !body.contains("code") ||
        !body.at("code").is_number_integer()) {
        throw std::runtime_error(
            "Govee control response is missing a valid code");
    }

    if (body.at("code") != 200) {
        std::string message =
            "Govee control failed (code " + body.at("code").dump() + ")";
        if (body.contains("message") && body.at("message").is_string()) {
            message += ": " + body.at("message").get<std::string>();
        }
        throw std::runtime_error(message);
    }
}

void save_devices_cache(const std::vector<Device>& devices) {
    auto entries = nlohmann::json::array();
    for (const auto& device : devices) {
        auto entry = nlohmann::json::object(
            {{"device", device.id}, {"sku", device.sku}});
        entries.push_back(entry);
    }

    std::ofstream cache_file("devices.json");
    if (!cache_file.is_open()) {
        throw std::runtime_error(
            "Error: Could not create or open device cache file");
    }

    cache_file << entries.dump(2);
    cache_file.close();

    if (cache_file.fail()) {
        throw std::runtime_error("Error: Could not write to device cache file");
    }
}

std::vector<Device> load_devices_cache() {
    std::ifstream cache_file("devices.json");
    if (!cache_file.is_open()) {
        throw std::runtime_error("Error: Could not load device cache file. Try "
                                 "running 'goveectl devices' first");
    }

    auto entries = nlohmann::json::parse(cache_file);
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

} // namespace govee
