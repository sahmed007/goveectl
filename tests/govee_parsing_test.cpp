#include "govee_internal.hpp"
#include <doctest/doctest.h>
#include <limits>
#include <stdexcept>

using govee::detail::parse_device_state;
using govee::detail::parse_response;
using nlohmann::json;

namespace {
json capability(const char* type, const char* instance, json value) {
    return {{"type", std::string("devices.capabilities.") + type},
            {"instance", instance},
            {"state", {{"value", value}}}};
}
json state_body(json capabilities) {
    return {{"payload", {{"capabilities", capabilities}}}};
}
} // namespace

TEST_CASE("API success preserves the response payload") {
    const json body = {{"code", 200}, {"data", json::array()}};
    CHECK(parse_response(200, body.dump()) == body);
}

TEST_CASE("HTTP failures take precedence over parsing the body") {
    for (long status : {401, 429, 500}) {
        CAPTURE(status);
        CHECK_THROWS_WITH_AS(parse_response(status, "not JSON"),
                             ("HTTP error: " + std::to_string(status)).c_str(),
                             std::runtime_error);
    }
}

TEST_CASE("API failures report available error messages") {
    CHECK_THROWS_WITH(
        parse_response(
            200, R"({"code":400,"message":"bad device","msg":"fallback"})"),
        "Govee API failed (code 400): bad device");
    CHECK_THROWS_WITH(
        parse_response(200, R"({"code":400,"message":null,"msg":"fallback"})"),
        "Govee API failed (code 400): fallback");
    CHECK_THROWS_WITH(parse_response(200, R"({"code":400})"),
                      "Govee API failed (code 400)");
}

TEST_CASE("invalid API envelopes are rejected") {
    CHECK_THROWS_WITH(parse_response(200, "{"),
                      "Govee API response was not valid JSON");
    for (const char* body : {"[]", "null", "{}", R"({"code":"200"})",
                             R"({"code":200.5})", R"({"code":true})"}) {
        CAPTURE(body);
        CHECK_THROWS_WITH(parse_response(200, body),
                          "Govee API response is missing a valid code");
    }
}

TEST_CASE("complete state preserves online and offline readings") {
    for (bool online : {true, false}) {
        const auto state = parse_device_state(state_body(json::array(
            {capability("online", "online", online),
             capability("on_off", "powerSwitch", 1),
             capability("range", "brightness", 50),
             capability("color_setting", "colorRgb", 0xFF8000),
             capability("color_setting", "colorTemperatureK", 2700)})));
        CHECK(state.online == online);
        CHECK(state.power == true);
        CHECK(state.brightness == 50);
        CHECK(state.color_rgb == 0xFF8000);
        CHECK(state.temperature_kelvin == 2700);
    }
}

TEST_CASE(
    "absent readings and unknown or incomplete capabilities remain unknown") {
    json caps = json::array();
    SUBCASE("empty capabilities") {}
    SUBCASE("unknown and incomplete entries") {
        caps = json::array({nullptr,
                            42,
                            json::object(),
                            {{"state", nullptr}},
                            {{"state", json::object()}},
                            capability("future", "newFeature", 123),
                            capability("range", "unrecognized", 50)});
    }
    const auto state = parse_device_state(state_body(caps));
    CHECK_FALSE(state.online.has_value());
    CHECK_FALSE(state.power.has_value());
    CHECK_FALSE(state.brightness.has_value());
    CHECK_FALSE(state.color_rgb.has_value());
    CHECK_FALSE(state.temperature_kelvin.has_value());
}

TEST_CASE("missing or malformed capabilities containers are rejected") {
    CHECK_THROWS_AS(parse_device_state(json::object()), json::out_of_range);
    CHECK_THROWS_AS(parse_device_state({{"payload", json::object()}}),
                    json::out_of_range);
    CHECK_THROWS_WITH(parse_device_state(state_body(json::object())),
                      "Expected a device state capabilities array");
}

TEST_CASE("state values reject invalid types and out of range readings") {
    for (const json& value :
         {json(nullptr), json("50"), json(1.5), json(-1), json(2147483648LL)}) {
        CAPTURE(value.dump());
        const auto state = parse_device_state(state_body(json::array(
            {capability("online", "online", value),
             capability("on_off", "powerSwitch", value),
             capability("range", "brightness", value),
             capability("color_setting", "colorRgb", value),
             capability("color_setting", "colorTemperatureK", value)})));
        CHECK_FALSE(state.online.has_value());
        CHECK_FALSE(state.power.has_value());
        CHECK_FALSE(state.brightness.has_value());
        CHECK_FALSE(state.color_rgb.has_value());
        CHECK_FALSE(state.temperature_kelvin.has_value());
    }
    const auto state = parse_device_state(state_body(
        json::array({capability("on_off", "powerSwitch", 2),
                     capability("range", "brightness", 101),
                     capability("color_setting", "colorRgb", 16777216),
                     capability("color_setting", "colorTemperatureK", 0)})));
    CHECK_FALSE(state.power.has_value());
    CHECK_FALSE(state.brightness.has_value());
    CHECK_FALSE(state.color_rgb.has_value());
    CHECK_FALSE(state.temperature_kelvin.has_value());
}

TEST_CASE("state accepts inclusive numeric boundaries") {
    for (int bound : {0, 1}) {
        const auto state = parse_device_state(state_body(json::array(
            {capability("on_off", "powerSwitch", bound),
             capability("range", "brightness", bound * 100),
             capability("color_setting", "colorRgb", bound * 16777215),
             capability("color_setting", "colorTemperatureK",
                        bound ? std::numeric_limits<int>::max() : 1)})));
        CHECK(state.power == (bound == 1));
        CHECK(state.brightness == bound * 100);
        CHECK(state.color_rgb == bound * 16777215);
        CHECK(state.temperature_kelvin ==
              (bound ? std::numeric_limits<int>::max() : 1));
    }
}
