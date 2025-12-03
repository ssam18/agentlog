cmake_minimum_required(VERSION 3.15)
project(AgentLogDemo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find AgentLog (assumes parent CMakeLists.txt is built)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Find dependencies
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

# Include AgentLog headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Link against AgentLog library
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../build/lib)

# Build payment demo (uses AgentLog library with AI features)
add_executable(payment_demo payment_demo.cpp)
target_link_libraries(payment_demo 
    PRIVATE 
    agentlog
    Threads::Threads
    CURL::libcurl
)
target_include_directories(payment_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Copy config file to build directory
configure_file(config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)

message(STATUS "AgentLog Demo Configuration:")
message(STATUS "  Build directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  AgentLog include: ${CMAKE_CURRENT_SOURCE_DIR}/../include")
message(STATUS "  AgentLog library: ${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")
