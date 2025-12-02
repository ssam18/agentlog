/**
 * @file basic_usage.cpp
 * @brief Basic usage example for AgentLog
 */

#include <agentlog/agentlog.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace agentlog;

int main() {
    // Initialize AgentLog
    Config config;
    config.service_name = "example-service";
    config.service_instance = "instance-1";
    config.enable_anomaly_detection = true;
    config.worker_threads = 2;
    
    global::init(config);
    
    // Simple text logging
    global::info("Application started");
    global::debug("Debug message");
    global::warn("Warning message");
    
    // Structured event logging
    AGENTLOG_EVENT("user.login")
        .entity("user_id", "user123")
        .entity("ip_address", "192.168.1.100")
        .context("user_agent", "Mozilla/5.0")
        .severity(Severity::INFO)
        .emit();
    
    // Business events with metrics
    AGENTLOG_EVENT("order.created")
        .entity("order_id", "order-456")
        .entity("customer_id", "cust-789")
        .metric("amount_usd", 149.99)
        .metric("items_count", 3.0)
        .context("payment_method", "credit_card")
        .emit();
    
    // Observe metrics (automatic anomaly detection)
    for (int i = 0; i < 100; ++i) {
        double latency = 50.0 + (i % 10) * 5.0;  // Normal: 50-95ms
        
        AGENTLOG_OBSERVE("api.latency")
            .metric("latency_ms", latency)
            .context("endpoint", "/api/users")
            .context("method", "GET")
            .emit();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Simulate anomaly - sudden latency spike
    AGENTLOG_OBSERVE("api.latency")
        .metric("latency_ms", 500.0)  // 10x normal!
        .context("endpoint", "/api/users")
        .context("method", "GET")
        .emit();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Error with stack trace
    AGENTLOG_EVENT("database.connection.failed")
        .severity(Severity::ERROR)
        .message("Failed to connect to database")
        .entity("database", "postgres-primary")
        .context("error_code", "ECONNREFUSED")
        .capture_stack_trace()
        .emit();
    
    // Register callback for anomalies
    Logger::instance().on_anomaly([](const LogEvent& event) {
        std::cout << "\n🚨 ANOMALY DETECTED 🚨\n";
        std::cout << event.to_json() << "\n\n";
    });
    
    // More observations to trigger detection
    for (int i = 0; i < 20; ++i) {
        double latency = 450.0 + (i % 5) * 10.0;  // Sustained high latency
        
        AGENTLOG_OBSERVE("api.latency")
            .metric("latency_ms", latency)
            .context("endpoint", "/api/users")
            .emit();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Display statistics
    auto stats = Logger::instance().get_stats();
    std::cout << "\n=== Statistics ===\n";
    std::cout << "Total events: " << stats.events_total << "\n";
    std::cout << "Anomalies detected: " << stats.anomalies_detected << "\n";
    std::cout << "Events dropped: " << stats.events_dropped << "\n";
    
    global::info("Application shutting down");
    global::shutdown();
    
    return 0;
}
