# Building AgentLog

This guide covers building AgentLog on Linux, macOS, and Windows.

## Quick Start

### Linux / macOS
```bash
./build.sh
sudo cmake --install build
```

### Windows (Visual Studio)
```cmd
build.bat --vs2022
cmake --install build --config Release
```

## Requirements

### All Platforms
- CMake 3.15 or higher
- C++17 compatible compiler
- libcurl (required)
- RocksDB (optional, for persistent storage)

### Linux
- GCC 7+ or Clang 6+
- Development packages: `libcurl4-openssl-dev`, `librocksdb-dev` (optional)

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libcurl4-openssl-dev

# Optional: RocksDB
sudo apt-get install librocksdb-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake libcurl-devel

# Optional: RocksDB
sudo dnf install rocksdb-devel
```

### macOS
- Xcode Command Line Tools or Xcode
- Homebrew (recommended for dependencies)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake curl

# Optional: RocksDB
brew install rocksdb
```

### Windows
- Visual Studio 2019/2022 **OR** MinGW-w64
- vcpkg (recommended for dependency management)

```cmd
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# Install dependencies
vcpkg install curl:x64-windows
vcpkg install rocksdb:x64-windows

# Set environment variable
set VCPKG_ROOT=C:\path\to\vcpkg
```

## Build Options

### Build Types
- `Debug` - Debug symbols, no optimization
- `Release` - Optimized, no debug symbols (default)
- `RelWithDebInfo` - Optimized with debug symbols
- `MinSizeRel` - Optimized for size

### CMake Options
- `AGENTLOG_BUILD_TESTS` - Build unit tests (ON by default)
- `AGENTLOG_BUILD_EXAMPLES` - Build example programs (ON by default)
- `AGENTLOG_HEADER_ONLY` - Header-only mode (OFF by default)
- `BUILD_SHARED_LIBS` - Build as shared library (OFF by default)

## Platform-Specific Builds

### Linux

#### Standard Build
```bash
./build.sh
```

#### Debug Build with Tests
```bash
./build.sh --type Debug
```

#### Release Build without Examples
```bash
./build.sh --type Release --no-examples
```

#### Custom Install Location
```bash
./build.sh --prefix $HOME/.local
```

#### Ninja Build System (faster)
```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
sudo ninja install
```

### macOS

#### Universal Binary (ARM64 + x86_64)
```bash
mkdir build && cd build
cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

#### Homebrew Installation Prefix
```bash
./build.sh --prefix /usr/local
```

### Windows

#### Visual Studio 2022
```cmd
build.bat --vs2022 --type Release
```

#### Visual Studio 2019
```cmd
build.bat --vs2019 --type Release
```

#### MinGW
```cmd
build.bat --mingw --type Release
```

#### With vcpkg
```cmd
set VCPKG_ROOT=C:\path\to\vcpkg
build.bat --vs2022
```

#### Manual CMake Configuration
```cmd
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
      -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cmake --install . --config Release
```

## Cross-Compilation

### Linux to ARM64
```bash
mkdir build-arm64 && cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm64-linux.cmake ..
make -j$(nproc)
```

### macOS to iOS
```bash
mkdir build-ios && cd build-ios
cmake -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 ..
xcodebuild -configuration Release
```

## Building from Source (Manual)

If you prefer manual control:

```bash
# Configure
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DAGENTLOG_BUILD_TESTS=ON \
      -DAGENTLOG_BUILD_EXAMPLES=ON \
      ..

# Build
cmake --build . --config Release -j$(nproc)

# Test
ctest --output-on-failure

# Install
sudo cmake --install . --config Release
```

## Using AgentLog in Your Project

### CMake (find_package)
```cmake
find_package(agentlog REQUIRED)
target_link_libraries(your_app PRIVATE agentlog::agentlog)
```

### CMake (FetchContent)
```cmake
include(FetchContent)
FetchContent_Declare(
    agentlog
    GIT_REPOSITORY https://github.com/ssam18/agentlog.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(agentlog)
target_link_libraries(your_app PRIVATE agentlog::agentlog)
```

### CMake (add_subdirectory)
```cmake
add_subdirectory(external/agentlog)
target_link_libraries(your_app PRIVATE agentlog::agentlog)
```

### pkg-config
```bash
g++ -std=c++17 myapp.cpp $(pkg-config --cflags --libs agentlog) -o myapp
```

## Troubleshooting

### libcurl not found (Linux)
```bash
sudo apt-get install libcurl4-openssl-dev  # Ubuntu/Debian
sudo dnf install libcurl-devel             # Fedora/RHEL
```

### libcurl not found (Windows)
```cmd
# Using vcpkg
vcpkg install curl:x64-windows
set VCPKG_ROOT=C:\path\to\vcpkg
```

### CMake version too old
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# Or download latest from cmake.org
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-Linux-x86_64.sh
sudo sh cmake-3.27.0-Linux-x86_64.sh --prefix=/usr/local --skip-license
```

### Compiler not C++17 compatible
```bash
# Update GCC
sudo apt-get install gcc-9 g++-9
export CXX=g++-9
export CC=gcc-9
```

### Build fails with "undefined reference to pthread_create"
```bash
# Make sure Threads::Threads is linked
cmake -DCMAKE_THREAD_PREFER_PTHREAD=ON ..
```

## Performance Tuning

### Link-Time Optimization (LTO)
```bash
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### Native CPU Optimization
```bash
cmake -DCMAKE_CXX_FLAGS="-march=native" ..
```

### Static Linking (portable binaries)
```bash
cmake -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" ..
```

## IDE Integration

### Visual Studio Code
```json
// .vscode/settings.json
{
    "cmake.configureSettings": {
        "AGENTLOG_BUILD_TESTS": "ON",
        "AGENTLOG_BUILD_EXAMPLES": "ON"
    }
}
```

### CLion
Open `CMakeLists.txt` as project. CLion will automatically detect configuration.

### Visual Studio
```cmd
cmake -G "Visual Studio 17 2022" -A x64 ..
# Open generated .sln file
```

## Continuous Integration

### GitHub Actions Example
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Dependencies (Linux)
      if: runner.os == 'Linux'
      run: sudo apt-get install -y libcurl4-openssl-dev
    
    - name: Install Dependencies (macOS)
      if: runner.os == 'macOS'
      run: brew install curl
    
    - name: Install Dependencies (Windows)
      if: runner.os == 'Windows'
      run: |
        vcpkg install curl:x64-windows
        echo "VCPKG_ROOT=$VCPKG_INSTALLATION_ROOT" >> $GITHUB_ENV
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        cmake --build . --config Release
    
    - name: Test
      run: |
        cd build
        ctest --output-on-failure --build-config Release
```

## Docker Build

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    build-essential cmake libcurl4-openssl-dev git
COPY . /agentlog
WORKDIR /agentlog
RUN ./build.sh --type Release
CMD ["/agentlog/build/bin/basic_usage"]
```

Build and run:
```bash
docker build -t agentlog .
docker run agentlog
```
