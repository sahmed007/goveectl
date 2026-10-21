#include "cli.hpp"
#include "commands.hpp"
#include "terminal.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    Options options;
    CLI::App app{"goveectl — control Govee devices from your terminal"};

    configure_cli(app, options);
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
        std::ostringstream output;
        std::ostringstream errors;
        const int status = app.exit(error, output, errors);
        std::cout << output.str();
        std::cerr << terminal::error(errors.str());
        return status;
    }

    const char* api_key = std::getenv("GOVEE_API_KEY");
    if (api_key == nullptr || api_key[0] == '\0') {
        std::cerr << terminal::error("GOVEE_API_KEY is not set") << '\n';
        return 1;
    }

    try {
        switch (options.command) {
        case Command::devices:
            if (!commands::show_devices(api_key)) {
                return 1;
            }
            break;
        case Command::status:
            commands::show_status(options.device, api_key);
            break;
        case Command::power_on:
            commands::change_power(options.device, true, api_key);
            break;
        case Command::power_off:
            commands::change_power(options.device, false, api_key);
            break;
        case Command::brightness:
            commands::change_brightness(options.device, options.brightness, api_key);
            break;
        case Command::color:
            commands::change_color(options.device, options.red, options.green, options.blue,
                                   api_key);
            break;
        case Command::temperature:
            commands::change_temperature(options.device, options.kelvin, api_key);
            break;
        case Command::none:
            std::cerr << terminal::error("No command selected") << '\n';
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << terminal::error(error.what()) << '\n';
        return 1;
    }

    return 0;
}
