#include "cli.hpp"
#include <doctest/doctest.h>

TEST_CASE("commands select the requested operation") {
    Options options;
    CLI::App app;
    configure_cli(app, options);

    SUBCASE("devices") {
        const char* argv[] = {"goveectl", "devices"};
        app.parse(2, argv);
        CHECK(options.command == Command::devices);
    }
    SUBCASE("status preserves a device name containing spaces") {
        const char* argv[] = {"goveectl", "status", "--device", "Desk Lamp"};
        app.parse(4, argv);
        CHECK(options.command == Command::status);
        CHECK(options.device == "Desk Lamp");
    }
    SUBCASE("power on") {
        const char* argv[] = {"goveectl", "power", "on", "-d", "lamp-id"};
        app.parse(5, argv);
        CHECK(options.command == Command::power_on);
        CHECK(options.device == "lamp-id");
    }
    SUBCASE("power off") {
        const char* argv[] = {"goveectl", "power", "off", "-d", "lamp-id"};
        app.parse(5, argv);
        CHECK(options.command == Command::power_off);
        CHECK(options.device == "lamp-id");
    }
}

TEST_CASE("brightness accepts the inclusive percentage boundaries") {
    for (const char* value : {"1", "100"}) {
        CAPTURE(value);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "brightness", value, "-d", "Desk Lamp"};

        app.parse(5, argv);

        CHECK(options.command == Command::brightness);
        CHECK(options.brightness == std::stoi(value));
        CHECK(options.device == "Desk Lamp");
    }
}

TEST_CASE("brightness rejects out-of-range and nonnumeric values") {
    for (const char* value : {"0", "101", "bright"}) {
        CAPTURE(value);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "brightness", value, "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(5, argv), CLI::ValidationError);
    }
}

TEST_CASE("hex and RGB forms produce the same orange color") {
    Options options;
    CLI::App app;
    configure_cli(app, options);

    SUBCASE("hex with hash") {
        const char* argv[] = {"goveectl", "color", "#FF8000", "-d", "lamp"};
        app.parse(5, argv);
    }
    SUBCASE("mixed-case hex without hash") {
        const char* argv[] = {"goveectl", "color", "fF8000", "-d", "lamp"};
        app.parse(5, argv);
    }
    SUBCASE("RGB channels") {
        const char* argv[] = {"goveectl", "color", "255", "128", "0", "-d", "lamp"};
        app.parse(7, argv);
    }

    CHECK(options.command == Command::color);
    CHECK(options.red == 255);
    CHECK(options.green == 128);
    CHECK(options.blue == 0);
    CHECK(options.device == "lamp");
}

TEST_CASE("each RGB channel accepts zero and 255") {
    for (const char* value : {"0", "255"}) {
        CAPTURE(value);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "color", value, value, value, "-d", "lamp"};
        app.parse(7, argv);
        CHECK(options.red == std::stoi(value));
        CHECK(options.green == std::stoi(value));
        CHECK(options.blue == std::stoi(value));
    }
}

TEST_CASE("malformed hex colors are rejected") {
    for (const char* value : {"#FFF", "1234567", "GG8000", "FF800Z"}) {
        CAPTURE(value);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "color", value, "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(5, argv), CLI::ValidationError);
    }
}

TEST_CASE("invalid RGB channels are rejected in every position") {
    for (const char* value : {"-1", "256", "red"}) {
        for (int channel = 2; channel <= 4; ++channel) {
            CAPTURE(value);
            CAPTURE(channel);
            Options options;
            CLI::App app;
            configure_cli(app, options);
            const char* argv[] = {"goveectl", "color", "0", "0", "0", "-d", "lamp"};
            argv[channel] = value;
            CHECK_THROWS_AS(app.parse(7, argv), CLI::ValidationError);
        }
    }
}

TEST_CASE("two color values are rejected") {
    Options options;
    CLI::App app;
    configure_cli(app, options);
    const char* argv[] = {"goveectl", "color", "255", "128", "-d", "lamp"};
    CHECK_THROWS_AS(app.parse(6, argv), CLI::ValidationError);
}

TEST_CASE("temperature accepts presets and numeric Kelvin") {
    struct Example {
        const char* input;
        int kelvin;
    };
    for (const auto& example :
         {Example{"warm", 2700}, Example{"neutral", 4000}, Example{"cool", 6500},
          Example{"WaRm", 2700}, Example{"NEUTRAL", 4000}, Example{"COOL", 6500},
          Example{"3500", 3500}}) {
        CAPTURE(example.input);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "temperature", example.input, "-d", "lamp"};
        app.parse(5, argv);
        CHECK(options.command == Command::temperature);
        CHECK(options.kelvin == example.kelvin);
        CHECK(options.device == "lamp");
    }
}

TEST_CASE("temperature rejects invalid Kelvin and unknown presets") {
    for (const char* value : {"0", "-1", "3500K", "3500.5", "hot"}) {
        CAPTURE(value);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", "temperature", value, "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(5, argv), CLI::ValidationError);
    }
}

TEST_CASE("control and status commands require a device") {
    for (const char* command : {"status", "brightness", "color", "temperature", "power"}) {
        CAPTURE(command);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const std::string name = command;
        const char* value = "50";
        if (name == "power")
            value = "on";
        else if (name == "color")
            value = "FF8000";
        const char* argv[] = {"goveectl", command, value};
        CHECK_THROWS_AS(app.parse(name == "status" ? 2 : 3, argv), CLI::RequiredError);
    }
    Options options;
    CLI::App app;
    configure_cli(app, options);
    const char* argv[] = {"goveectl", "power", "off"};
    CHECK_THROWS_AS(app.parse(3, argv), CLI::RequiredError);
}

TEST_CASE("value-taking commands require their value") {
    for (const char* command : {"brightness", "color", "temperature"}) {
        CAPTURE(command);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* argv[] = {"goveectl", command, "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(4, argv), CLI::RequiredError);
    }
}

TEST_CASE("missing commands and unknown options are rejected") {
    Options options;
    CLI::App app;
    configure_cli(app, options);
    SUBCASE("no command") {
        const char* argv[] = {"goveectl"};
        CHECK_THROWS_AS(app.parse(1, argv), CLI::RequiredError);
    }
    SUBCASE("power requires on or off") {
        const char* argv[] = {"goveectl", "power"};
        CHECK_THROWS_AS(app.parse(2, argv), CLI::RequiredError);
    }
    SUBCASE("unknown option") {
        const char* argv[] = {"goveectl", "devices", "--bogus"};
        CHECK_THROWS_AS(app.parse(3, argv), CLI::ExtrasError);
    }
}

TEST_CASE("help and version request successful early exits") {
    Options options;
    CLI::App app;
    configure_cli(app, options);
    SUBCASE("help") {
        const char* argv[] = {"goveectl", "--help"};
        CHECK_THROWS_AS(app.parse(2, argv), CLI::CallForHelp);
    }
    SUBCASE("subcommand help does not require a device") {
        const char* argv[] = {"goveectl", "brightness", "--help"};
        CHECK_THROWS_AS(app.parse(3, argv), CLI::CallForHelp);
    }
    SUBCASE("version") {
        const char* argv[] = {"goveectl", "--version"};
        CHECK_THROWS_AS(app.parse(2, argv), CLI::CallForVersion);
    }
    CHECK(options.command == Command::none);
}

TEST_CASE("numeric overflow is rejected before any command runs") {
    for (const char* command : {"brightness", "temperature", "color"}) {
        CAPTURE(command);
        Options options;
        CLI::App app;
        configure_cli(app, options);
        const char* huge = "999999999999999999999999999999";
        const char* argv[] = {"goveectl", command, huge, "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(5, argv), CLI::ParseError);
    }
    Options options;
    CLI::App app;
    configure_cli(app, options);
    const char* argv[] = {"goveectl", "color", "999999999999999999999", "0", "0", "-d", "lamp"};
    CHECK_THROWS_AS(app.parse(7, argv), CLI::ValidationError);
}

TEST_CASE("unexpected positional arguments are rejected") {
    Options options;
    CLI::App app;
    configure_cli(app, options);
    SUBCASE("devices accepts no positional arguments") {
        const char* argv[] = {"goveectl", "devices", "extra"};
        CHECK_THROWS_AS(app.parse(3, argv), CLI::ExtrasError);
    }
    SUBCASE("brightness accepts one value") {
        const char* argv[] = {"goveectl", "brightness", "50", "60", "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(6, argv), CLI::ExtrasError);
    }
    SUBCASE("color accepts at most three values") {
        const char* argv[] = {"goveectl", "color", "1", "2", "3", "4", "-d", "lamp"};
        CHECK_THROWS_AS(app.parse(8, argv), CLI::ParseError);
    }
}
