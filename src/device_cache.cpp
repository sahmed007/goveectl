#include "device_json.hpp" // IWYU pragma: keep
#include "govee.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

namespace govee {

namespace {
std::filesystem::path cache_path() {
    const char* base_dir = nullptr;
    std::filesystem::path suffix;
#if defined(_WIN32)
    base_dir = std::getenv("LOCALAPPDATA");
#elif defined(__APPLE__)
    base_dir = std::getenv("HOME");
    suffix = std::filesystem::path{"Library"} / "Caches";
#else
    const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
    if (xdg_cache != nullptr && xdg_cache[0] != '\0' &&
        std::filesystem::path{xdg_cache}.is_absolute()) {
        return std::filesystem::path{xdg_cache} / "goveectl" / "devices.json";
    }
    base_dir = std::getenv("HOME");
    suffix = ".cache";
#endif
    if (base_dir == nullptr || base_dir[0] == '\0' ||
        !std::filesystem::path{base_dir}.is_absolute()) {
        throw std::runtime_error(
            "Could not determine an absolute user cache directory");
    }
    return std::filesystem::path{base_dir} / suffix / "goveectl" /
           "devices.json";
}
} // namespace

void save_devices_cache(const std::vector<Device>& devices) {
    const nlohmann::json entries = devices;

    const auto path = cache_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream cache_file(path);
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
    std::ifstream cache_file(cache_path());
    if (!cache_file.is_open()) {
        throw std::runtime_error("Error: Could not load device cache file. Try "
                                 "running 'goveectl devices' first");
    }

    auto entries = nlohmann::json::parse(cache_file);
    if (!entries.is_array()) {
        throw std::runtime_error("Expected a device array");
    }

    return entries.get<std::vector<Device>>();
}

} // namespace govee
