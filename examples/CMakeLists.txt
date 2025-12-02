cmake_minimum_required(VERSION 3.15)

# Basic usage example
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE agentlog)

# Payment service example
add_executable(payment_service payment_service.cpp)
target_link_libraries(payment_service PRIVATE agentlog)

# Pattern detection example (Phase 2)
add_executable(pattern_detection pattern_detection.cpp)
target_link_libraries(pattern_detection PRIVATE agentlog)

# Microservices correlation example (Phase 2)
add_executable(microservices_correlation microservices_correlation.cpp)
target_link_libraries(microservices_correlation PRIVATE agentlog)

# Integration demo example (Phase 3)
add_executable(integration_demo integration_demo.cpp)
target_link_libraries(integration_demo PRIVATE agentlog)

# Integration test (Phase 3)
add_executable(test_integrations test_integrations.cpp)
target_link_libraries(test_integrations PRIVATE agentlog)

# Install examples (optional)
install(TARGETS basic_usage payment_service pattern_detection microservices_correlation integration_demo test_integrations
    RUNTIME DESTINATION bin/examples
)
