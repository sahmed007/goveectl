#include "cli.hpp"
#include "terminal.hpp"
#include <charconv>
#include <cctype>
#include <memory>
#include <sstream>
#include <string_view>

constexpr inline std::string_view GOVEECTL_VERSION = "0.1";

namespace {
int parse_temperature(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    if (text == "warm") return 2700;
    if (text == "neutral") return 4000;
    if (text == "cool") return 6500;

    int kelvin = 0;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), kelvin);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        kelvin <= 0) {
        throw CLI::ValidationError(
            "temperature", "Use warm, neutral, cool, or a positive integer in Kelvin");
    }
    return kelvin;
}

int parse_hex_color(std::string_view hex) {
    if (hex.starts_with('#')) {
        hex.remove_prefix(1);
    }
    unsigned int rgb = 0;
    const auto result =
        std::from_chars(hex.data(), hex.data() + hex.size(), rgb, 16);
    if (hex.size() != 6 || result.ec != std::errc{} ||
        result.ptr != hex.data() + hex.size()) {
        throw CLI::ValidationError(
            "color", "Expected six hex digits, optionally prefixed with # "
                     "(e.g. \"#FF8000\")");
    }
    return static_cast<int>(rgb);
}

int parse_rgb_channel(std::string_view text) {
    int value = 0;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        value < 0 || value > 255) {
        throw CLI::ValidationError(
            "color", "RGB channels must be integers from 0 to 255");
    }
    return value;
}

void parse_color(Options& options) {
    const auto& values = options.color_values;
    if (values.size() == 1) {
        const int rgb = parse_hex_color(values.front());
        options.red = (rgb >> 16) & 255;
        options.green = (rgb >> 8) & 255;
        options.blue = rgb & 255;
        return;
    }
    if (values.size() == 3) {
        options.red = parse_rgb_channel(values[0]);
        options.green = parse_rgb_channel(values[1]);
        options.blue = parse_rgb_channel(values[2]);
        return;
    }
    throw CLI::ValidationError("color",
                               "Supply one hex color or three RGB integers");
}

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
  goveectl status -d "Desk Lamp"
  goveectl temperature warm -d "Desk Lamp"
  goveectl brightness 50 -d "Desk Lamp"
  goveectl color 255 128 0 -d "Desk Lamp"

DEVICE SELECTION
  --device accepts an exact, case-sensitive Govee Home name or device ID.
  Quote names containing spaces. If names are duplicated, use an ID.
  Run 'goveectl devices' again after adding or renaming a device.

CACHE
  Control and status commands resolve devices from the cache.
  Status makes one state request; devices refreshes the list and queries each device.
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
        "devices", "Refresh the cache and show live connectivity and power");

    devices_subcommand->footer(R"(EXAMPLE
  goveectl devices

Requires GOVEE_API_KEY and an internet connection. Run this once before
controlling devices, and again after adding or renaming a device.
The cache is shared across working directories. Makes one list request plus
one state request per device. Offline power is shown as Unknown.
Failed state queries show Error; other rows are still displayed.
Any failed state query causes a nonzero exit code.)");

    devices_subcommand->callback(
        [&options] { options.command = Command::devices; });

    CLI::App* status_subcommand = app.add_subcommand(
        "status",
        "Query a device's connectivity, power, brightness, and color");
    status_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();
    status_subcommand->callback(
        [&options] { options.command = Command::status; });
    status_subcommand->footer(R"(EXAMPLE
  goveectl status -d "Desk Lamp"

Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Queries state once. Missing values display Unknown.
Offline readings are labeled last-known; unknown connectivity is unverified.
Names are case-sensitive; use an ID if a name matches multiple devices.)");

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

    CLI::App* temperature_subcommand =
        app.add_subcommand("temperature", "Set white color temperature in Kelvin");
    temperature_subcommand
        ->add_option("value", options.temperature_input,
                     "warm (2700 K), neutral (4000 K), cool (6500 K), or Kelvin")
        ->required();
    temperature_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();
    temperature_subcommand->callback([&options] {
        options.kelvin = parse_temperature(options.temperature_input);
        options.command = Command::temperature;
    });
    temperature_subcommand->footer(R"(EXAMPLES
  goveectl temperature warm -d "Desk Lamp"
  goveectl temperature cool -d "Desk Lamp"
  goveectl temperature 3500 -d "Desk Lamp"

Presets are case-insensitive: warm = 2700 K, neutral = 4000 K, cool = 6500 K.
Numeric input is a positive integer in Kelvin, without a K suffix.
Requires a device supporting color temperature. Supported ranges vary by device;
Govee validates the requested value. Values are never silently clamped.
Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.)");

    // CHANGE COLOR
    CLI::App* color_subcommand =
        app.add_subcommand("color", "Change a device's color");

    color_subcommand
        ->add_option("color", options.color_values,
                     "Hex RRGGBB / #RRGGBB, or three RGB integers (0-255)")
        ->expected(1, 3)
        ->required();

    color_subcommand
        ->add_option("-d,--device", options.device,
                     "Target device ID or exact name (case-sensitive)")
        ->required();

    color_subcommand->footer(R"(EXAMPLES
  goveectl color "#FF8000" -d "Desk Lamp"   # Orange
  goveectl color ff8000 -d "Desk Lamp"      # Same, without #
  goveectl color 255 0 0 -d "Desk Lamp"      # Red
  goveectl color 0 255 0 -d "Desk Lamp"      # Green
  goveectl color 0 0 255 -d "Desk Lamp"      # Blue
  goveectl color 255 128 0 -d "Desk Lamp"    # Orange

Supply six hex digits (case-insensitive, optional #), or three integers in
RED GREEN BLUE order, each from 0 to 255. Quote colors starting with #.
Requires GOVEE_API_KEY and a cache created with 'goveectl devices'.
Names are case-sensitive; use an ID if a name matches multiple devices.)");

    color_subcommand->callback([&options]() {
        parse_color(options);
        options.command = Command::color;
    });
};
