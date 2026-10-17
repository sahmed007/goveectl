#include "cli.hpp"
#include <string_view>

constexpr inline std::string_view GOVEECTL_VERSION = "0.1";

void configure_cli(CLI::App& app, Options& options) {
    app.set_version_flag("-v,--version", std::string{GOVEECTL_VERSION});
    app.require_subcommand(1);

    // LIST DEVICES
    CLI::App* devices_subcommand =
        app.add_subcommand("devices", "List all available devices");

    devices_subcommand->callback(
        [&options] { options.command = Command::devices; });

    // CHANGE BRIGHTNESS
    CLI::App* brightness_subcommand =
        app.add_subcommand("brightness", "Change a device's brightness");

    brightness_subcommand
        ->add_option("value", options.brightness, "Brightness level")
        ->required()
        ->check(CLI::Range(1, 100));

    brightness_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    brightness_subcommand->callback(
        [&options]() { options.command = Command::brightness; });

    // CONTROL POWER
    CLI::App* power_subcommand =
        app.add_subcommand("power", "Control a device's power")
            ->require_subcommand(1);

    CLI::App* on_subcommand =
        power_subcommand->add_subcommand("on", "Turn a device on");

    on_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    on_subcommand->callback(
        [&options] { options.command = Command::power_on; });

    CLI::App* off_subcommand =
        power_subcommand->add_subcommand("off", "Turn a device off");

    off_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    off_subcommand->callback(
        [&options] { options.command = Command::power_off; });

    // CHANGE COLOR
    CLI::App* color_subcommand =
        app.add_subcommand("color", "Change a device's color");

    color_subcommand->add_option("r", options.red, "Red channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("g", options.green, "Green channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("b", options.blue, "Blue channel")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    color_subcommand->callback(
        [&options]() { options.command = Command::color; });
};
