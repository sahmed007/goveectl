#pragma once

#include "govee.hpp"
#include <nlohmann/json.hpp>

namespace govee {

inline void to_json(nlohmann::json& json, const Device& device) {
    json = {{"sku", device.sku},
            {"device", device.id},
            {"deviceName", device.name}};
}

inline void from_json(const nlohmann::json& json, Device& device) {
    device = Device{.sku = json.at("sku").get<std::string>(),
                    .id = json.at("device").get<std::string>(),
                    .name = json.value("deviceName", std::string{})};
}

} // namespace govee
