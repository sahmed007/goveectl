#pragma once

#include "CLI11.hpp"
#include <string>
#include <vector>

enum class Command {
    none,
    devices,
    status,
    power_on,
    power_off,
    brightness,
    color,
    temperature
};

struct Options {
    Command command = Command::none;
    std::string device;
    int brightness = 0;
    std::string temperature_input;
    int kelvin = 0;
    int red = 0;
    int green = 0;
    int blue = 0;
    std::vector<std::string> color_values;
};

void configure_cli(CLI::App& app, Options& options);
