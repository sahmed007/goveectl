#include "CLI11.hpp"
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

    int brightness_value;
    brightness_subcommand->add_option("-c,--change", brightness_value,
                                      "The brightness value to change to");

    CLI11_PARSE(app, argc, argv);

    if (devices_subcommand->parsed()) {
        std::cout << "Fetching devices...\n";
    }

    if (on_subcommand->parsed()) {
        std::cout << "Turning on " << device_option << "\n";
    } else if (off_subcommand->parsed()) {
        std::cout << "Turning off " << device_option << "\n";
    }

    if (brightness_subcommand->parsed()) {
        std::cout << "Changing brightness to " << brightness_value << "\n";
    }

    std::cout << "goveectl is running\n";
    return 0;
}
