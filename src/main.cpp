#include "CLI11.hpp"
#include <iostream>

int main(int argc, char **argv) {
  CLI::App app{"Govee Cloud CLI"};

  CLI11_PARSE(app, argc, argv);

  std::cout << "goveectl is running\n";
  return 0;
}
