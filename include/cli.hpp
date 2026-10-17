#pragma once

#include "CLI11.hpp"
#include <string>

enum class Command { none, devices, status, power_on, power_off, brightness, color };

struct Options {
    Command command = Command::none;
    std::string device;
    int brightness = 0;
    int red = 0;
    int green = 0;
    int blue = 0;
};

void configure_cli(CLI::App& app, Options& options);
