#include "govee.hpp"
#include "test_helpers.hpp"
#include <doctest/doctest.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
struct CacheFixture {
    test::TemporaryDirectory directory;
#ifdef _WIN32
    test::Environment base{"LOCALAPPDATA", directory.path.string()};
    std::filesystem::path file = directory.path / "goveectl/devices.json";
#elif defined(__APPLE__)
    test::Environment base{"HOME", directory.path.string()};
    std::filesystem::path file = directory.path / "Library/Caches/goveectl/devices.json";
#else
    test::Environment base{"HOME", directory.path.string()};
    test::Environment xdg{"XDG_CACHE_HOME", directory.path.string()};
    std::filesystem::path file = directory.path / "goveectl/devices.json";
#endif
    void write(const std::string& text) {
        std::filesystem::create_directories(file.parent_path());
        std::ofstream output(file);
        output << text;
        output.close();
        if (!output)
            throw std::runtime_error("Could not write test fixture");
    }
};
} // namespace

TEST_CASE_FIXTURE(CacheFixture, "cache creates directories and round trips device fields") {
    CHECK_FALSE(std::filesystem::exists(file.parent_path()));
    govee::save_devices_cache({{"H1", "id-1", "Desk \"Lamp\""}, {"H2", "id-2", ""}});
    CHECK(std::filesystem::is_regular_file(file));
    const auto devices = govee::load_devices_cache();
    REQUIRE(devices.size() == 2);
    CHECK(devices[0].sku == "H1");
    CHECK(devices[0].id == "id-1");
    CHECK(devices[0].name == "Desk \"Lamp\"");
    CHECK(devices[1].sku == "H2");
    CHECK(devices[1].id == "id-2");
    CHECK(devices[1].name.empty());
    govee::save_devices_cache({});
    CHECK(govee::load_devices_cache().empty());
}

TEST_CASE_FIXTURE(CacheFixture, "missing cache explains how to initialize it") {
    CHECK_THROWS_WITH(govee::load_devices_cache(),
                      "Error: Could not load device cache file. Try running "
                      "'goveectl devices' first");
}

TEST_CASE_FIXTURE(CacheFixture, "cache rejects corrupt JSON and invalid device records") {
    SUBCASE("malformed JSON") {
        write("[");
        CHECK_THROWS_AS(govee::load_devices_cache(), nlohmann::json::parse_error);
    }
    SUBCASE("non-array") {
        write("{}");
        CHECK_THROWS_WITH(govee::load_devices_cache(), "Expected a device array");
    }
    SUBCASE("missing SKU") {
        write(R"([{"device":"id-1"}])");
        CHECK_THROWS_AS(govee::load_devices_cache(), nlohmann::json::out_of_range);
    }
    SUBCASE("missing device ID") {
        write(R"([{"sku":"H1"}])");
        CHECK_THROWS_AS(govee::load_devices_cache(), nlohmann::json::out_of_range);
    }
    SUBCASE("incorrect field type") {
        write(R"([{"sku":"H1","device":42}])");
        CHECK_THROWS_AS(govee::load_devices_cache(), nlohmann::json::type_error);
    }
}

TEST_CASE_FIXTURE(CacheFixture, "cache allows a missing optional device name") {
    write(R"([{"sku":"H1","device":"id-1"}])");
    const auto devices = govee::load_devices_cache();
    REQUIRE(devices.size() == 1);
    CHECK(devices.front().name.empty());
}
