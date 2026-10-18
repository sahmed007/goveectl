#include "govee_internal.hpp"
#include <doctest/doctest.h>
#include <stdexcept>

using govee::detail::find_device;

TEST_CASE("devices resolve by exact ID or case sensitive name") {
    const std::vector<govee::Device> devices = {{"H1", "id-1", "Desk Lamp"},
                                                {"H2", "id-2", "Shelf"}};
    CHECK(find_device(devices, "id-1").name == "Desk Lamp");
    CHECK(find_device(devices, "Shelf").id == "id-2");
    for (const char* selector : {"desk lamp", "Desk", "missing", ""}) {
        CAPTURE(selector);
        CHECK_THROWS_WITH_AS(
            find_device(devices, selector),
            "Device name or ID not found in cache (names are case-sensitive). "
            "Run 'goveectl devices' to refresh.",
            std::runtime_error);
    }
    CHECK_THROWS_AS(find_device({}, "id-1"), std::runtime_error);
    CHECK_THROWS_AS(find_device({{"H1", "id-1", ""}}, ""), std::runtime_error);
}

TEST_CASE("duplicate names require an ID and ID matches take precedence") {
    const std::vector<govee::Device> devices = {{"H1", "id-1", "Lamp"},
                                                {"H2", "id-2", "Lamp"},
                                                {"H3", "id-3", "id-1"},
                                                {"H4", "id-4", "id-1"}};
    CHECK_THROWS_WITH(find_device(devices, "Lamp"),
                      "Multiple devices named 'Lamp'. Use a device ID from "
                      "'goveectl devices'.");
    CHECK(find_device(devices, "id-1").sku == "H1");
    CHECK(find_device(devices, "id-2").sku == "H2");
}
