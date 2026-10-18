#include "display.hpp"
#include "test_helpers.hpp"
#include <doctest/doctest.h>

TEST_CASE("empty device lists give an explicit message") {
    test::CaptureOutput output;
    display::print_devices({});
    CHECK(output.str() == "No devices found.\n");
}

TEST_CASE("device table shows unnamed devices and query failures") {
    test::CaptureOutput output;
    display::print_devices({{{"H1", "id-1", ""}, {}, true}});
    const auto text = output.str();
    CHECK(text.find("(unnamed)") != std::string::npos);
    CHECK(text.find("id-1") != std::string::npos);
    CHECK(text.find("H1") != std::string::npos);
    std::istringstream rows(text);
    std::string header, name, model, status, power, id;
    std::getline(rows, header);
    rows >> name >> model >> status >> power >> id;
    CHECK(status == "Error");
    CHECK(power == "Error");
}

TEST_CASE(
    "device table hides stale power for offline and unknown connectivity") {
    for (const auto online :
         {std::optional<bool>{false}, std::optional<bool>{}}) {
        test::CaptureOutput output;
        display::print_devices({{{"H1", "id-1", "Lamp"},
                                 {.online = online, .power = true},
                                 false}});
        std::istringstream rows(output.str());
        std::string header, name, model, status, power, id;
        std::getline(rows, header);
        rows >> name >> model >> status >> power >> id;
        CHECK(status == (online.has_value() ? "Offline" : "Unknown"));
        CHECK(power == "Unknown");
    }
}

TEST_CASE("status labels missing readings as unknown and unverified") {
    test::CaptureOutput output;
    display::print_status({"H1", "id-1", ""}, {});
    const auto text = output.str();
    CHECK(text.find("Name: (unnamed)") != std::string::npos);
    CHECK(text.find("Status: Unknown") != std::string::npos);
    for (const char* field : {"Power", "Brightness", "Color", "Temperature"}) {
        CHECK(text.find(std::string(field) + " (unverified): Unknown") !=
              std::string::npos);
    }
}

TEST_CASE(
    "status formats zero brightness and padded color with offline qualifiers") {
    for (bool online : {true, false}) {
        test::CaptureOutput output;
        display::print_status({"H1", "id-1", "Lamp"},
                              {.online = online,
                               .power = false,
                               .brightness = 0,
                               .color_rgb = 0xAB,
                               .temperature_kelvin = 2700});
        const auto text = output.str();
        const std::string qualifier = online ? "" : " (last-known)";
        CHECK(text.find("Power" + qualifier + ": Off") != std::string::npos);
        CHECK(text.find("Brightness" + qualifier + ": 0%") !=
              std::string::npos);
        CHECK(text.find("Color" + qualifier + ": #0000AB") !=
              std::string::npos);
        CHECK(text.find("Temperature" + qualifier + ": 2700 K") !=
              std::string::npos);
    }
}
