cmake_minimum_required(VERSION 3.15)
project(agentlog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(AGENTLOG_BUILD_TESTS "Build tests" ON)
option(AGENTLOG_BUILD_EXAMPLES "Build examples" ON)
option(AGENTLOG_ENABLE_AI "Enable AI/ML features" ON)
option(AGENTLOG_ENABLE_TELEMETRY "Enable OpenTelemetry integration" OFF)

# Dependencies
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

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
    target_link_libraries(agentlog PUBLIC Threads::Threads CURL::libcurl)
    
    # Set compiler warnings
    if(MSVC)
        target_compile_options(agentlog PRIVATE /W4)
    else()
        target_compile_options(agentlog PRIVATE -Wall -Wextra -Wpedantic)
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
install(TARGETS agentlog
    EXPORT agentlog-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/agentlog
    DESTINATION include
)

install(EXPORT agentlog-targets
    FILE agentlog-config.cmake
    NAMESPACE agentlog::
    DESTINATION lib/cmake/agentlog
)
