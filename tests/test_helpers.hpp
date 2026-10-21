#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace test {
class Environment {
    std::string name_;
    std::optional<std::string> previous_;
    static void set(const std::string& name, const char* value) {
#ifdef _WIN32
        if (_putenv_s(name.c_str(), value ? value : "") != 0)
#else
        if ((value ? setenv(name.c_str(), value, 1) : unsetenv(name.c_str())) != 0)
#endif
            throw std::runtime_error("Could not set test environment");
    }

  public:
    Environment(std::string name, const std::string& value) : name_(std::move(name)) {
        if (const char* old = std::getenv(name_.c_str()))
            previous_ = old;
        set(name_, value.c_str());
    }
    ~Environment() { set(name_, previous_ ? previous_->c_str() : nullptr); }
    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;
};

class TemporaryDirectory {
  public:
    std::filesystem::path path;
    TemporaryDirectory() {
        std::random_device random;
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = std::filesystem::temp_directory_path() /
                   ("goveectl-test-" + std::to_string(random()));
            if (std::filesystem::create_directory(path))
                return;
        }
        throw std::runtime_error("Could not create test directory");
    }
    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
};

class CaptureOutput {
    Environment no_color_{"NO_COLOR", "1"};
    std::ostringstream output_;
    std::streambuf* previous_ = std::cout.rdbuf(output_.rdbuf());

  public:
    ~CaptureOutput() { std::cout.rdbuf(previous_); }
    std::string str() const { return output_.str(); }
    CaptureOutput() = default;
    CaptureOutput(const CaptureOutput&) = delete;
    CaptureOutput& operator=(const CaptureOutput&) = delete;
};
} // namespace test
