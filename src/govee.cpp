#include "govee.hpp"
#include "cpr/cprtypes.h"
#include "cpr/error.h"
#include "cpr/response.h"
#include "cpr/timeout.h"
#include <cpr/cpr.h>
#include <stdexcept>
#include <string>

namespace govee {

std::string list_devices(const std::string& api_key) {
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

    return response.text;
}

} // namespace govee
