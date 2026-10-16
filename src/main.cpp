#include "CLI11.hpp"
#include "cli.hpp"
#include "govee.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

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
            std::cout << govee::list_devices(api_key) << '\n';
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
        break;
    case Command::power_on:
        std::cout << "Turning on " << options.device << "\n";
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
