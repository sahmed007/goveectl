#include "CLI11.hpp"
#include "govee.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

constexpr inline std::string_view GOVEECTL_VERSION = "0.1";

int main(int argc, char** argv) {
    CLI::App app{"Govee Cloud CLI"};

    app.set_version_flag("-v,--version", std::string{GOVEECTL_VERSION});

    CLI::App* devices_subcommand =
        app.add_subcommand("devices", "List all available devices");

    CLI::App* power_subcommand =
        app.add_subcommand("power", "Control a device's power")
            ->require_subcommand(1);

    CLI::App* on_subcommand =
        power_subcommand->add_subcommand("on", "Turn a device on");

    CLI::App* off_subcommand =
        power_subcommand->add_subcommand("off", "Turn a device off");

    std::string device_option;

    on_subcommand
        ->add_option("-d,--device", device_option, "The specified device")
        ->required();

    off_subcommand
        ->add_option("-d,--device", device_option, "The specified device")
        ->required();

    CLI::App* brightness_subcommand =
        app.add_subcommand("brightness", "Change a device's brightness");

    int brightness_value{0};

    brightness_subcommand
        ->add_option("value", brightness_value, "Brightness level")
        ->required()
        ->check(CLI::Range(0, 100));

    brightness_subcommand
        ->add_option("-d,--device", device_option, "Target device")
        ->required();

    CLI::App* color_subcommand =
        app.add_subcommand("color", "Change a device's color");

    int red = {0};
    int green = {0};
    int blue = {0};

    color_subcommand->add_option("r", red, "Red channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("g", green, "Green channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("b", blue, "Blue channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand
        ->add_option("-d,--device", device_option, "The specified device")
        ->required();

    app.require_subcommand(1);

    CLI11_PARSE(app, argc, argv);

    const char* api_key = std::getenv("GOVEE_API_KEY");
    if (api_key == nullptr || api_key[0] == '\0') {
        std::cerr << "GOVEE_API_KEY is not set\n";
        return 1;
    }

    if (devices_subcommand->parsed()) {
        try {
            std::cout << govee::list_devices(api_key) << '\n';
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }

    if (on_subcommand->parsed()) {
        std::cout << "Turning on " << device_option << "\n";
    } else if (off_subcommand->parsed()) {
        std::cout << "Turning off " << device_option << "\n";
    }

    if (brightness_subcommand->parsed()) {
        std::cout << "Changing brightness of " << device_option << " to "
                  << brightness_value << "%\n";
    }

    if (color_subcommand->parsed()) {
        std::cout << "Setting color to RGB(" << red << ", " << green << ", "
                  << blue << ")\n";
    }

    return 0;
}
