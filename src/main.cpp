#include "CLI11.hpp"
#include "cli.hpp"
#include "govee.hpp"
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    Options options;
    CLI::App app{"Govee Cloud CLI"};

    configure_cli(app, options);
    CLI11_PARSE(app, argc, argv);

    const char* api_key = std::getenv("GOVEE_API_KEY");
    if (api_key == nullptr || api_key[0] == '\0') {
        std::cerr << "GOVEE_API_KEY is not set\n";
        return 1;
    }

    switch (options.command) {
    case Command::devices:
        try {
            std::vector<govee::Device> devices = govee::list_devices(api_key);
            govee::save_devices_cache(devices);
            for (const auto& d : devices) {
                std::cout << "Device SKU: " << d.sku << " and "
                          << "Device ID: " << d.id << '\n';
            }
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
        break;
    case Command::power_on:
        try {
            std::vector<govee::Device> devices = govee::load_devices_cache();
            auto it =
                std::ranges::find(devices, options.device, &govee::Device::id);
            if (it == devices.end()) {
                throw std::runtime_error("Device not found in cache. Run "
                                         "'goveectl devices' to refresh.");
            }
            std::cout << "Found device: " << it->sku << " " << it->id << '\n';
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
        break;
    case Command::power_off:
        std::cout << "Turning off " << options.device << "\n";
        break;
    case Command::brightness:
        std::cout << "Changing brightness of " << options.device << " to "
                  << options.brightness << "%\n";
        break;
    case Command::color:
        std::cout << "Setting color to RGB(" << options.red << ", "
                  << options.green << ", " << options.blue << ")\n";
        break;
    case Command::none:
        std::cerr << "No command selected" << '\n';
        return 1;
    }

    return 0;
}
