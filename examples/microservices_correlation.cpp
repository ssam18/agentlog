// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

/**
 * @file microservices_correlation.cpp
 * @brief Demonstrates AgentLog Phase 2 correlation and causality analysis
 * 
 * This example simulates a microservices architecture with:
 * - Multiple services (API Gateway, Auth, Database, Payment)
 * - Trace ID correlation across services
 * - Entity-based correlation (users, orders)
 * - Causality detection (root cause analysis)
 * 
 * Note: This is a simplified demo. Real correlation analysis requires
 * including correlation_engine.h and incident_manager.h headers.
 */

#include <agentlog/agentlog.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>

using namespace agentlog;
using namespace agentlog::global;
using namespace std::chrono_literals;

// Generate random trace IDs
std::string generate_trace_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    uint64_t id = dis(gen);
    char buf[32];
    snprintf(buf, sizeof(buf), "trace-%016lx", id);
    return buf;
}

void simulate_successful_request(const std::string& trace_id, uint64_t user_id) {
    std::cout << "\n--- Simulating Successful Request: " << trace_id << " ---\n";
    
    std::string user_str = "user-" + std::to_string(user_id);
    
    // 1. API Gateway receives request
    AGENTLOG_EVENT("api.request.received")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("endpoint", "/api/orders")
        .context("method", "POST")
        .severity(Severity::INFO)
        .emit();
    
    std::this_thread::sleep_for(50ms);
    
    // 2. Auth service validates token
    AGENTLOG_EVENT("auth.token.validated")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("token_expiry", "3600s")
        .severity(Severity::INFO)
        .emit();
    
    std::this_thread::sleep_for(100ms);
    
    // 3. Database query
    AGENTLOG_EVENT("database.query.executed")
        .context("trace_id", trace_id)
        .entity("database_name", "orders-db")
        .metric("query_time_ms", 45.0)
        .context("query_type", "INSERT")
        .severity(Severity::INFO)
        .emit();
    
    std::this_thread::sleep_for(100ms);
    
    // 4. Response sent
    AGENTLOG_EVENT("api.response.sent")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("status_code", "200")
        .metric("total_latency_ms", 195.0)
        .severity(Severity::INFO)
        .emit();
}

void simulate_cascading_failure_with_correlation(const std::string& trace_id, uint64_t user_id) {
    std::cout << "\n--- Simulating Cascading Failure: " << trace_id << " ---\n";
    
    std::string user_str = "user-" + std::to_string(user_id);
    
    // 1. API Gateway receives request
    AGENTLOG_EVENT("api.request.received")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("endpoint", "/api/payments")
        .context("method", "POST")
        .severity(Severity::INFO)
        .emit();
    
    std::this_thread::sleep_for(50ms);
    
    // 2. Auth service validates (success)
    AGENTLOG_EVENT("auth.token.validated")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .severity(Severity::INFO)
        .emit();
    
    std::this_thread::sleep_for(100ms);
    
    // 3. Database starts slowing down (ROOT CAUSE)
    AGENTLOG_EVENT("database.slow.query")
        .context("trace_id", trace_id)
        .entity("database_name", "payments-db")
        .metric("query_time_ms", 4500.0)  // Very slow!
        .context("query_type", "SELECT")
        .context("table", "payment_methods")
        .severity(Severity::WARNING)
        .emit();
    
    std::this_thread::sleep_for(500ms);  // Shortened for demo
    
    // 4. API times out due to slow database
    AGENTLOG_EVENT("api.timeout")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("timeout_ms", "5000")
        .context("dependent_service", "database")
        .severity(Severity::ERROR)
        .emit();
    
    std::this_thread::sleep_for(100ms);
    
    // 5. Payment fails
    AGENTLOG_EVENT("payment.processing.failed")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("reason", "upstream_timeout")
        .context("amount", "99.99")
        .severity(Severity::ERROR)
        .emit();
    
    std::this_thread::sleep_for(50ms);
    
    // 6. User sees error
    AGENTLOG_EVENT("user.request.failed")
        .context("trace_id", trace_id)
        .entity("user_id", user_str)
        .context("status_code", "503")
        .context("error_message", "Service Unavailable")
        .severity(Severity::ERROR)
        .emit();
}

void simulate_concurrent_users() {
    std::cout << "\n=== Simulating Concurrent Users ===\n";
    
    // Multiple users making requests simultaneously
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i]() {
            std::string trace_id = generate_trace_id();
            uint64_t user_id = 1000 + i;
            
            if (i == 1) {
                // One request fails
                simulate_cascading_failure_with_correlation(trace_id, user_id);
            } else {
                // Others succeed
                simulate_successful_request(trace_id, user_id);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    // Initialize with correlation enabled
    Config config;
    config.service_name = "microservices-demo";
    config.environment = "production";
    config.enable_pattern_matching = true;
    config.enable_correlation = true;
    config.enable_anomaly_detection = true;
    config.enable_auto_incidents = true;
    config.incident_anomaly_threshold = 0.7;
    
    global::init(config);
    global::info("Microservices correlation demo started");
    
    // Note: Correlation analysis happens automatically in the background
    // Events with the same trace_id are automatically correlated
    // Causality relationships (database.slow -> api.timeout -> user.error) are detected
    
    // Scenario 1: Single successful request
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "Scenario 1: Successful Request\n";
    std::cout << "════════════════════════════════════════\n";
    simulate_successful_request(generate_trace_id(), 5000);
    std::this_thread::sleep_for(2s);
    
    // Scenario 2: Cascading failure with root cause
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "Scenario 2: Cascading Failure (Root Cause: Slow DB)\n";
    std::cout << "════════════════════════════════════════\n";
    simulate_cascading_failure_with_correlation(generate_trace_id(), 5001);
    std::this_thread::sleep_for(2s);
    
    // Scenario 3: Concurrent users
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "Scenario 3: Concurrent Users (Mixed Success/Failure)\n";
    std::cout << "════════════════════════════════════════\n";
    simulate_concurrent_users();
    std::this_thread::sleep_for(2s);
    
    // Print statistics
    std::cout << "\n════════════════════════════════════════\n";
    std::cout << "=== Microservices Correlation Summary ===\n";
    std::cout << "════════════════════════════════════════\n";
    
    auto stats = Logger::instance().get_stats();
    std::cout << "Total events: " << stats.events_total << "\n";
    std::cout << "Anomalies: " << stats.anomalies_detected << "\n";
    std::cout << "Patterns: " << stats.patterns_matched << "\n";
    std::cout << "Correlations: " << stats.correlations_found << "\n";
    
    std::cout << "\nNote: Events with the same trace_id are automatically correlated.\n";
    std::cout << "Causality chains (database.slow → api.timeout → user.error) are detected.\n";
    
    return 0;
}
