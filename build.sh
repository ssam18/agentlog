#!/bin/bash

# AgentLog Build Script for Multi-Platform Support
# Supports: Linux, macOS, Windows (via MinGW or MSVC)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX=""
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
CLEAN=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to display usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build AgentLog library for multiple platforms

OPTIONS:
    -h, --help              Show this help message
    -t, --type TYPE         Build type: Debug, Release, RelWithDebInfo (default: Release)
    -d, --dir DIR           Build directory (default: build)
    -p, --prefix PATH       Installation prefix
    -j, --jobs N            Number of parallel jobs (default: auto-detected)
    --no-tests              Disable building tests
    --no-examples           Disable building examples
    --clean                 Clean build directory before building
    --shared                Build as shared library (default: static)
    --header-only           Build as header-only library

EXAMPLES:
    # Build with default settings
    $0

    # Debug build with 8 parallel jobs
    $0 --type Debug --jobs 8

    # Build and install to custom location
    $0 --prefix /usr/local

    # Clean build for distribution
    $0 --clean --type Release --no-tests

EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --shared)
            BUILD_SHARED="ON"
            shift
            ;;
        --header-only)
            HEADER_ONLY="ON"
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    PLATFORM="Windows"
else
    PLATFORM="Unknown"
fi

print_info "Platform detected: $PLATFORM"
print_info "Build type: $BUILD_TYPE"
print_info "Build directory: $BUILD_DIR"
print_info "Parallel jobs: $JOBS"

# Clean build directory if requested
if $CLEAN && [ -d "$BUILD_DIR" ]; then
    print_warning "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Prepare CMake arguments
CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DAGENTLOG_BUILD_TESTS=$BUILD_TESTS"
    "-DAGENTLOG_BUILD_EXAMPLES=$BUILD_EXAMPLES"
)

if [ -n "$INSTALL_PREFIX" ]; then
    CMAKE_ARGS+=("-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX")
fi

if [ -n "$BUILD_SHARED" ]; then
    CMAKE_ARGS+=("-DBUILD_SHARED_LIBS=ON")
fi

if [ -n "$HEADER_ONLY" ]; then
    CMAKE_ARGS+=("-DAGENTLOG_HEADER_ONLY=ON")
fi

# Platform-specific configuration
if [ "$PLATFORM" == "Windows" ]; then
    print_info "Configuring for Windows..."
    # Add Windows-specific flags if needed
elif [ "$PLATFORM" == "macOS" ]; then
    print_info "Configuring for macOS..."
    # Add macOS-specific flags if needed
fi

# Run CMake configuration
print_info "Running CMake configuration..."
cmake "${CMAKE_ARGS[@]}" .. || {
    print_error "CMake configuration failed"
    exit 1
}

# Build
print_info "Building with $JOBS parallel jobs..."
cmake --build . --config "$BUILD_TYPE" -j "$JOBS" || {
    print_error "Build failed"
    exit 1
}

# Run tests if enabled
if [ "$BUILD_TESTS" == "ON" ]; then
    print_info "Running tests..."
    ctest --output-on-failure --build-config "$BUILD_TYPE" || {
        print_warning "Some tests failed"
    }
fi

print_info "Build completed successfully!"
print_info ""
print_info "To install, run:"
print_info "  cd $BUILD_DIR && sudo cmake --install ."
print_info ""
print_info "To run examples:"
print_info "  cd $BUILD_DIR/bin"
print_info "  ./basic_usage"
