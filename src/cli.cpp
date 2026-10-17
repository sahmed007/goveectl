#include "cli.hpp"
#include "terminal.hpp"
#include <memory>
#include <sstream>
#include <string_view>

constexpr inline std::string_view GOVEECTL_VERSION = "0.1";

namespace {
class ColorFormatter : public CLI::Formatter {
  public:
    std::string make_help(const CLI::App* app, std::string name,
                          CLI::AppFormatMode mode) const override {
        const auto plain = CLI::Formatter::make_help(app, name, mode);
        if (!terminal::supports_color(stdout)) {
            return plain;
        }
        std::istringstream input{plain};
        std::string result;
        std::string line;
        while (std::getline(input, line)) {
            const auto start = line.find_first_not_of(" \t");
            const auto end = line.find_last_not_of(" \t");
            const auto text = start == std::string::npos
                                  ? std::string{}
                                  : line.substr(start, end - start + 1);
            if (text == "OPTIONS:" || text == "POSITIONALS:" ||
                text == "SUBCOMMANDS:" || text == "SETUP" ||
                text == "EXAMPLE" || text == "EXAMPLES" ||
                text == "DEVICE SELECTION" || text == "CACHE" ||
                text == "MORE HELP") {
                line = terminal::style(line, "1");
            } else if (text.starts_with("goveectl ") ||
                       text.find(" [OPTIONS]") != std::string::npos) {
                line = terminal::style(line, "36");
            }
            result += line + '\n';
        }
        return result;
    }
};
} // namespace

void configure_cli(CLI::App& app, Options& options) {
    app.formatter(std::make_shared<ColorFormatter>());
    app.set_version_flag("-v,--version", std::string{GOVEECTL_VERSION});
    app.require_subcommand(1);
    app.footer(R"(SETUP
  Set GOVEE_API_KEY to your Govee developer API key, then run: 'goveectl devices'.
  This fetches device names and IDs and refreshes the local cache.

EXAMPLES
  goveectl power on -d "Desk Lamp"
  goveectl power off -d "Desk Lamp"
  goveectl brightness 50 -d "Desk Lamp"
  goveectl color 255 128 0 -d "Desk Lamp"

DEVICE SELECTION
  --device accepts an exact, case-sensitive Govee Home name or device ID.
  Quote names containing spaces. If names are duplicated, use an ID.
  Run 'goveectl devices' again after adding or renaming a device.

CACHE
  Control commands read the cache; they do not fetch the device list.
  macOS:   ~/Library/Caches/goveectl/devices.json
  Linux:   $XDG_CACHE_HOME/goveectl/devices.json
           (default: ~/.cache/goveectl/devices.json)
  Windows: %LOCALAPPDATA%\goveectl\devices.json

MORE HELP
  goveectl power on --help
  goveectl brightness --help
  goveectl color --help
  Help and version output do not require an API key.)");

    // LIST DEVICES
    CLI::App* devices_subcommand = app.add_subcommand(
        "devices", "Fetch device names and IDs and refresh the cache");

    devices_subcommand->footer(R"(EXAMPLE
  goveectl devices

Requires GOVEE_API_KEY and an internet connection. Run this once before
controlling devices, and again after adding or renaming a device.
The cache is shared across working directories.)");

    devices_subcommand->callback(
        [&options] { options.command = Command::devices; });

    // CHANGE BRIGHTNESS
    CLI::App* brightness_subcommand =
        app.add_subcommand("brightness", "Change a device's brightness");

    brightness_subcommand
        ->add_option("value", options.brightness,
                     "Brightness percentage (1-100)")
        ->required()
        ->check(CLI::Range(1, 100));

    brightness_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    brightness_subcommand->footer(R"(EXAMPLE
  goveectl brightness 50 -d "Desk Lamp"

Sets an absolute brightness percentage. Use 'power off' to turn a light off.
Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Names are case-sensitive; use an ID if a name matches multiple devices.)");

    brightness_subcommand->callback(
        [&options]() { options.command = Command::brightness; });

    // CONTROL POWER
    CLI::App* power_subcommand =
        app.add_subcommand("power", "Control a device's power")
            ->require_subcommand(1);

    power_subcommand->footer(R"(EXAMPLES
  goveectl power on -d "Desk Lamp"
  goveectl power off -d "Desk Lamp"

Choose on or off, then provide --device on that subcommand.
Run 'goveectl power on --help' for its options.)");

    CLI::App* on_subcommand =
        power_subcommand->add_subcommand("on", "Turn a device on");

    on_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    on_subcommand->footer(R"(EXAMPLE
  goveectl power on -d "Desk Lamp"

Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Names are case-sensitive; quote names containing spaces.
Use a device ID if a name matches multiple devices.)");

    on_subcommand->callback(
        [&options] { options.command = Command::power_on; });

    CLI::App* off_subcommand =
        power_subcommand->add_subcommand("off", "Turn a device off");

    off_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    off_subcommand->footer(R"(EXAMPLE
  goveectl power off -d "Desk Lamp"

Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Names are case-sensitive; quote names containing spaces.
Use a device ID if a name matches multiple devices.)");

    off_subcommand->callback(
        [&options] { options.command = Command::power_off; });

    // CHANGE COLOR
    CLI::App* color_subcommand =
        app.add_subcommand("color", "Change a device's color");

    color_subcommand->add_option("r", options.red, "Red channel (0-255)")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("g", options.green, "Green channel (0-255)")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand->add_option("b", options.blue, "Blue channel (0-255)")
        ->required()
        ->check(CLI::Range(0, 255));

    color_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    color_subcommand->footer(R"(EXAMPLES
  goveectl color 255 0 0 -d "Desk Lamp"      # Red
  goveectl color 0 255 0 -d "Desk Lamp"      # Green
  goveectl color 0 0 255 -d "Desk Lamp"      # Blue
  goveectl color 255 128 0 -d "Desk Lamp"    # Orange

Supply three integers in RED GREEN BLUE order, each from 0 to 255.
Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Names are case-sensitive; use an ID if a name matches multiple devices.)");

    color_subcommand->callback(
        [&options]() { options.command = Command::color; });
};
