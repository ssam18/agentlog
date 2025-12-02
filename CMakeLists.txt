cmake_minimum_required(VERSION 3.15)
project(agentlog VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    set(AGENTLOG_PLATFORM "Windows")
    add_definitions(-DAGENTLOG_PLATFORM_WINDOWS)
elseif(APPLE)
    set(AGENTLOG_PLATFORM "macOS")
    add_definitions(-DAGENTLOG_PLATFORM_MACOS)
elseif(UNIX)
    set(AGENTLOG_PLATFORM "Linux")
    add_definitions(-DAGENTLOG_PLATFORM_LINUX)
endif()

message(STATUS "Building AgentLog for ${AGENTLOG_PLATFORM}")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Options
option(AGENTLOG_BUILD_TESTS "Build tests" ON)
option(AGENTLOG_BUILD_EXAMPLES "Build examples" ON)
option(AGENTLOG_ENABLE_AI "Enable AI/ML features" ON)
option(AGENTLOG_ENABLE_TELEMETRY "Enable OpenTelemetry integration" OFF)

# Dependencies
find_package(Threads REQUIRED)

# Find CURL with platform-specific hints
if(WIN32)
    # On Windows, look for curl in vcpkg or system
    find_package(CURL REQUIRED)
    if(CURL_FOUND)
        message(STATUS "Found CURL: ${CURL_LIBRARIES}")
    endif()
else()
    find_package(CURL REQUIRED)
    if(CURL_FOUND)
        message(STATUS "Found CURL: ${CURL_LIBRARIES}")
    endif()
endif()

# Optional: RocksDB for persistent storage
find_package(RocksDB QUIET)
if(RocksDB_FOUND)
    message(STATUS "Found RocksDB: ${RocksDB_LIBRARIES}")
    add_definitions(-DAGENTLOG_HAVE_ROCKSDB)
else()
    message(STATUS "RocksDB not found - persistent storage disabled")
endif()

# Header-only option for fast compilation during development
option(AGENTLOG_HEADER_ONLY "Header-only mode" OFF)

# Include directories
include_directories(include)

# Core library sources
set(AGENTLOG_SOURCES
    src/event.cpp
    src/logger.cpp
    src/ingestion.cpp
    src/anomaly_detector.cpp
    src/pattern_engine.cpp
    src/correlation_engine.cpp
    src/incident_manager.cpp
    src/storage.cpp
    src/curl_helper.cpp
)

if(NOT AGENTLOG_HEADER_ONLY)
    add_library(agentlog ${AGENTLOG_SOURCES})
    target_include_directories(agentlog PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    
    # Link libraries
    target_link_libraries(agentlog PUBLIC Threads::Threads CURL::libcurl)
    
    # Optional RocksDB
    if(RocksDB_FOUND)
        target_link_libraries(agentlog PUBLIC RocksDB::rocksdb)
    endif()
    
    # Platform-specific libraries
    if(WIN32)
        target_link_libraries(agentlog PUBLIC ws2_32 wldap32 crypt32 normaliz)
    endif()
    
    # Set compiler warnings and flags
    if(MSVC)
        target_compile_options(agentlog PRIVATE /W4 /utf-8)
        target_compile_definitions(agentlog PRIVATE _CRT_SECURE_NO_WARNINGS)
        # Enable multi-processor compilation
        target_compile_options(agentlog PRIVATE /MP)
    else()
        target_compile_options(agentlog PRIVATE -Wall -Wextra -Wpedantic)
        # Enable position independent code for shared libraries
        set_target_properties(agentlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    
    # Export symbols on Windows
    if(WIN32)
        set_target_properties(agentlog PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
else()
    add_library(agentlog INTERFACE)
    target_include_directories(agentlog INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endif()

# Examples
if(AGENTLOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(AGENTLOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS agentlog
    EXPORT agentlog-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/agentlog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Generate and install CMake config files
include(CMakePackageConfigHelpers)

# Create config version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/agentlog-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install targets export
install(EXPORT agentlog-targets
    FILE agentlog-targets.cmake
    NAMESPACE agentlog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/agentlog
)

# Create config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/agentlog-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/agentlog-config.cmake"
    @ONLY
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/agentlog-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/agentlog-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/agentlog
)

# Print summary
message(STATUS "")
message(STATUS "AgentLog Configuration Summary:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Platform:             ${AGENTLOG_PLATFORM}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:         ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests:          ${AGENTLOG_BUILD_TESTS}")
message(STATUS "  Build examples:       ${AGENTLOG_BUILD_EXAMPLES}")
message(STATUS "  Header-only:          ${AGENTLOG_HEADER_ONLY}")
message(STATUS "  CURL found:           ${CURL_FOUND}")
message(STATUS "  RocksDB found:        ${RocksDB_FOUND}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
