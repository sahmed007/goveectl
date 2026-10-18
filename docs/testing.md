# Testing goveectl

[← Back to the README](../README.md)

## Build and run

You need a C++20 compiler, CMake 3.21 or newer, Git, and system libcurl
development files. Run these commands from the repository directory in a POSIX
shell. The first configuration downloads pinned dependencies,
including doctest when tests are enabled.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest runs the `goveectl_tests` executable. To see individual doctest cases or
run just one, invoke it directly:

```sh
./build/goveectl_tests --list-test-cases
./build/goveectl_tests --test-case="hex and RGB forms produce the same orange color"
```

The tests exercise CLI parsing, API response and state parsing, device selection,
cache persistence, and display output without an API key, network requests, or
real lights. Cache tests use temporary directories and restore environment
variables; they never read or write your real device cache. They work in Release
builds too. To build only the application, configure with `-DBUILD_TESTING=OFF`; doctest is then not fetched.

## How the tests work

`tests/main.cpp` defines `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` exactly once,
which supplies the test runner's `main()`. The test files compile alongside the
real offline application code, separately from the application's `main()` and
HTTP transport. Internal parsing and selection helpers are shared with the
application so tests exercise the same behavior.

CLI tests create fresh `Options` and `CLI::App` objects, parse an argument
array, and check the result. Other suites use JSON fixtures, temporary cache
files, or captured terminal output. This is the arrange, act, assert pattern:

- `TEST_CASE` names a behavior to verify.
- `CHECK` reports a failed assertion and continues checking that test.
- `REQUIRE` stops the current test on failure. Use it before accessing something
  that must exist, such as checking a vector is nonempty before indexing it.
- `CHECK_THROWS_AS` checks that an expression throws the expected exception.
- `SUBCASE` explores a variation; doctest reruns the surrounding test setup for
  each path, so the variations do not share parsed state.
- `CAPTURE` adds the current input to failure messages, making loops over cases
  easier to debug.

When adding behavior, add a small test with representative valid inputs and
invalid or boundary inputs. Keep real-device checks separate from this suite.
