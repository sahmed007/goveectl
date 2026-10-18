# goveectl

![goveectl — Command-line control for Govee devices](docs/assets/banner.svg)

[![C++: 20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat&logo=cplusplus&logoColor=white)](CMakeLists.txt)
[![CMake: ≥ 3.21](https://img.shields.io/badge/CMake-%E2%89%A5%203.21-064F8C?style=flat&logo=cmake&logoColor=white)](#1-build-from-source)
[![Tests: doctest](https://img.shields.io/badge/Tests-doctest-398447?style=flat)](docs/testing.md)

goveectl is a C++20 command-line interface for discovering and controlling Govee
devices through the Govee cloud API. It supports device status queries, power
control, brightness, RGB color, and color temperature.

Device operations require a Govee API key and an internet connection. Available
controls depend on the device.

[Quick start](#quick-start) · [Commands](#commands) · [Usage examples](#usage-examples) ·
[Troubleshooting](#troubleshooting) · [Development](#development)

## Quick start

### 1. Build from source

Requirements: a **C++20 compiler**, **CMake 3.21+**, **Git**, and **libcurl
development files**. The commands below use a POSIX shell, such as bash or zsh. Start in the
repository directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --parallel
```

The first configuration downloads the project's dependencies. This builds the
CLI without the test runner; contributor instructions are [below](#development).

Add the build directory to PATH for the current shell session:

```sh
export PATH="$PWD/build:$PATH"
goveectl --help
```

Alternatively, use `./build/goveectl` from the repository directory in place of
`goveectl` in the examples below.

### 2. Configure the API key

Obtain an API key through the Govee Home app using
[Govee's official instructions](https://developer.govee.com/reference/apply-you-govee-api-key).
Set it in the same shell, replacing the placeholder:

```sh
export GOVEE_API_KEY='your-api-key-here'
```

Keep the key private. goveectl reads this environment variable; it does **not**
automatically load a `.env` file.

### 3. Discover and control a device

Fetch the device list and initialize the local cache:

```sh
goveectl devices
```

Select a device name from the output. Replace `"Desk Lamp"` throughout these
examples with that name:

```sh
goveectl power on -d "Desk Lamp" &&
goveectl color "#FF8000" -d "Desk Lamp"
```

## Commands

Every control and status command takes `-d` / `--device`. Use an **exact,
case-sensitive name** or a device ID from `goveectl devices`. Quote names with
spaces. If two devices share a name, use their IDs.

| Operation | Example | Notes |
| --- | --- | --- |
| Device discovery | `goveectl devices` | Refreshes the cache; shows connectivity and power. |
| Device status | `goveectl status -d "Desk Lamp"` | Shows power, brightness, color, and temperature. |
| Power | `goveectl power on -d "Desk Lamp"` | Replace `on` with `off` to switch off. |
| Brightness | `goveectl brightness 50 -d "Desk Lamp"` | Accepts 1–100; use `power off` to turn off. |
| RGB color (hex) | `goveectl color "#FF8000" -d "Desk Lamp"` | Six hex digits; the `#` is optional. Quote it when included. |
| RGB color (channels) | `goveectl color 255 128 0 -d "Desk Lamp"` | Red, green, blue: each 0–255. |
| Color temperature | `goveectl temperature warm -d "Desk Lamp"` | Also accepts `neutral`, `cool`, or a Kelvin value such as `3500`. |

Temperature presets are **warm = 2700 K**, **neutral = 4000 K**, and
**cool = 6500 K**. Preset names are case-insensitive. Numeric temperatures must
be positive integers without a `K` suffix. Supported ranges depend on the device;
Govee validates the request; goveectl does not clamp the value.

Help and version output are available without an API key:

```sh
goveectl --help
goveectl color --help
goveectl power on --help
goveectl --version
```

## Usage examples

These shell command sequences can be used interactively or in scripts. The `&&`
operator prevents subsequent commands from running if a request fails.

### Set warm white lighting

```sh
goveectl power on -d "Desk Lamp" &&
goveectl temperature warm -d "Desk Lamp" &&
goveectl brightness 25 -d "Desk Lamp"
```

### Set an RGB color and brightness

```sh
goveectl power on -d "Desk Lamp" &&
goveectl color "#8B5CF6" -d "Desk Lamp" &&
goveectl brightness 40 -d "Desk Lamp"
```

## Troubleshooting

### Device discovery and cached state

Run `goveectl devices` once before controlling lights, and again after adding or
renaming a device. It caches names and IDs across working directories, makes
one discovery request, then queries each device's state. `status` queries only
the selected device.

Offline status readings are labeled **last-known**; readings with unknown
connectivity are **unverified**. Missing values display **Unknown**. In the device
list, offline power is shown as Unknown, and failed state queries show **Error**
while other rows are still displayed.

| Issue | Resolution |
| --- | --- |
| `GOVEE_API_KEY is not set` | Export the key in the shell running goveectl. A `.env` file alone does not set it. |
| Cache missing or JSON parsing failed while loading it | Run `goveectl devices` to recreate the cache. |
| Device name or ID not found | Refresh with `goveectl devices`, then copy the exact name or ID. Names are case-sensitive. |
| Multiple devices have the same name | Use the device ID with `-d`. |
| Temperature request rejected | Check that the device supports color temperature and that the value is in its supported range. |

<details>
<summary>Cache locations</summary>

The cache stores device names, IDs, and models, not your API key.

| Platform | Location |
| --- | --- |
| macOS | `~/Library/Caches/goveectl/devices.json` |
| Linux | `$XDG_CACHE_HOME/goveectl/devices.json`, or `~/.cache/goveectl/devices.json` by default |
| Windows | `%LOCALAPPDATA%\goveectl\devices.json` |

</details>

**Scripting notes:** failed commands return a nonzero exit status. That includes
`devices` when any state query fails. Set `NO_COLOR=1` to disable terminal colors;
redirected output already omits them. Output is intended for people; there is no
JSON output mode.

## Development

Contributions are welcome. For bug fixes, include reproduction steps and a
focused test covering the affected behavior.

From the repository directory:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The tests cover CLI parsing, API responses, device selection, cache persistence,
and display output. They make no network requests, need no credentials or real
lights, and use temporary directories instead of your device cache.

For test filtering, fixtures, and doctest usage, see the
[testing guide](docs/testing.md).
