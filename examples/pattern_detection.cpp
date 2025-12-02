// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

/**
 * @file pattern_detection.cpp
 * @brief Demonstrates AgentLog Phase 2 pattern detection capabilities
 * 
 * This example shows:
 * - Sequential patterns (cascading failures)
 * - Frequency patterns (auth failure bursts, retry storms)
 * - Pattern matching callbacks
 */

#include <agentlog/agentlog.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace agentlog;
using namespace agentlog::global;
using namespace std::chrono_literals;

void simulate_cascading_failure() {
    std::cout << "\n=== Simulating Cascading Failure ===\n";
    
    // This should trigger cascading_failure pattern:
    // database.error -> api.timeout -> user.error
    
    AGENTLOG_EVENT("database.connection.failed")
        .entity("database_id", "postgres-primary")
        .context("error_code", "CONNECTION_TIMEOUT")
        .context("retries", "3")
        .severity(Severity::ERROR)
        .emit();
    
    std::this_thread::sleep_for(200ms);
    
    AGENTLOG_EVENT("api.timeout")
        .entity("api_endpoint", "/api/orders")
        .context("timeout_ms", "5000")
        .context("dependent_service", "database")
        .severity(Severity::ERROR)
        .emit();
    
    std::this_thread::sleep_for(200ms);
    
    AGENTLOG_EVENT("user.request.failed")
        .entity("user_id", "user@example.com")
        .context("status_code", "503")
        .context("error", "Service Unavailable")
        .severity(Severity::ERROR)
        .emit();
}

void simulate_auth_failure_burst() {
    std::cout << "\n=== Simulating Authentication Failure Burst ===\n";
    
    // This should trigger auth_failure_pattern:
    // 5+ auth failures within 60 seconds
    
    for (int i = 0; i < 7; ++i) {
        AGENTLOG_EVENT("auth.login.failed")
            .entity("user_email", "attacker" + std::to_string(i) + "@malicious.com")
            .context("reason", "invalid_password")
            .context("ip_address", "192.168.1." + std::to_string(100 + i))
            .context("attempt", std::to_string(i + 1))
            .severity(Severity::WARNING)
            .emit();
        
        std::this_thread::sleep_for(500ms);
    }
}

void simulate_retry_storm() {
    std::cout << "\n=== Simulating Retry Storm ===\n";
    
    // This should trigger retry_storm pattern:
    // 10+ retries within 30 seconds
    
    for (int i = 0; i < 12; ++i) {
        AGENTLOG_EVENT("service.retry")
            .entity("service_name", "payment-processor")
            .context("operation", "process_payment")
            .context("retry_count", std::to_string(i + 1))
            .context("backoff_ms", std::to_string(100 * (i + 1)))
            .severity(Severity::WARNING)
            .emit();
        
        std::this_thread::sleep_for(200ms);
    }
}

void simulate_exception_burst() {
    std::cout << "\n=== Simulating Exception Burst ===\n";
    
    // This should trigger exception_pattern:
    // Multiple exceptions in short time
    
    for (int i = 0; i < 5; ++i) {
        AGENTLOG_EVENT("application.exception")
            .context("exception_type", "NullPointerException")
            .context("method", "OrderService.processOrder()")
            .context("line", std::to_string(142 + i))
            .context("stack_depth", "8")
            .severity(Severity::ERROR)
            .emit();
        
        std::this_thread::sleep_for(300ms);
    }
}

void simulate_memory_leak_pattern() {
    std::cout << "\n=== Simulating Memory Leak Pattern ===\n";
    
    // This should trigger memory_leak_pattern:
    // Steadily increasing memory usage
    
    for (int i = 0; i < 8; ++i) {
        double memory_mb = 512.0 + (i * 64.0);  // 512MB -> 960MB
        
        AGENTLOG_EVENT("system.memory.high")
            .metric("memory_used_mb", memory_mb)
            .metric("memory_percent", 50.0 + (i * 5.0))
            .context("process", "worker-pool")
            .context("heap_size_mb", std::to_string(memory_mb))
            .severity(Severity::WARNING)
            .emit();
        
        std::this_thread::sleep_for(1s);
    }
}

int main() {
    // Initialize with pattern matching enabled
    Config config;
    config.service_name = "pattern-detection-demo";
    config.environment = "development";
    config.enable_pattern_matching = true;
    config.enable_correlation = true;
    config.enable_anomaly_detection = true;
    
    global::init(config);
    global::info("Pattern detection demo started");
    
    // Note: Pattern matching happens automatically in the background
    // Patterns are logged when detected
    
    // Scenario 1: Cascading Failure
    simulate_cascading_failure();
    std::this_thread::sleep_for(2s);
    
    // Scenario 2: Authentication Failure Burst (security threat)
    simulate_auth_failure_burst();
    std::this_thread::sleep_for(2s);
    
    // Scenario 3: Retry Storm (service degradation)
    simulate_retry_storm();
    std::this_thread::sleep_for(2s);
    
    // Scenario 4: Exception Burst (code issue)
    simulate_exception_burst();
    std::this_thread::sleep_for(2s);
    
    // Scenario 5: Memory Leak Detection
    simulate_memory_leak_pattern();
    std::this_thread::sleep_for(2s);
    
    // Print statistics
    std::cout << "\n=== Pattern Detection Summary ===\n";
    
    auto stats = Logger::instance().get_stats();
    std::cout << "Total events: " << stats.events_total << "\n";
    std::cout << "Anomalies: " << stats.anomalies_detected << "\n";
    std::cout << "Patterns: " << stats.patterns_matched << "\n";
    std::cout << "Correlations: " << stats.correlations_found << "\n";
    
    return 0;
}
