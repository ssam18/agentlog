/**
 * @file payment_service.cpp
 * @brief Real-world payment service monitoring example
 */

#include <agentlog/agentlog.h>
#include <iostream>
#include <random>
#include <thread>

using namespace agentlog;

// Simulate payment processing
struct PaymentResult {
    bool success;
    double processing_time_ms;
    std::string error_code;
};

PaymentResult process_payment(const std::string& order_id, double amount) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> latency_dist(50.0, 150.0);
    static std::uniform_int_distribution<> success_dist(1, 100);
    
    PaymentResult result;
    result.processing_time_ms = latency_dist(gen);
    result.success = success_dist(gen) > 5;  // 95% success rate
    
    if (!result.success) {
        result.error_code = "PAYMENT_DECLINED";
    }
    
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(result.processing_time_ms))
    );
    
    return result;
}

int main() {
    // Initialize with payment service configuration
    Config config;
    config.service_name = "payment-service";
    config.service_instance = "pod-7f8a9b";
    config.environment = "production";
    config.enable_anomaly_detection = true;
    config.enable_auto_incidents = true;
    config.incident_anomaly_threshold = 0.75;
    
    global::init(config);
    global::info("Payment service started");
    
    // Register anomaly handler for critical payments
    Logger::instance().on_anomaly([](const LogEvent& event) {
        if (event.severity() >= Severity::ERROR) {
            std::cout << "\n🔥 CRITICAL PAYMENT ANOMALY 🔥\n";
            std::cout << "Event: " << event.event_type() << "\n";
            std::cout << "Anomaly Score: " << event.anomaly_score() << "\n";
            std::cout << event.to_json() << "\n\n";
            
            // In production: trigger PagerDuty, create Jira ticket, etc.
        }
    });
    
    // Simulate payment processing
    for (int i = 1; i <= 200; ++i) {
        std::string order_id = "ORD-" + std::to_string(1000 + i);
        double amount = 50.0 + (i % 10) * 10.0;
        
        // Start transaction
        AGENTLOG_EVENT("payment.transaction.started")
            .entity("order_id", order_id)
            .metric("amount_usd", amount)
            .context("currency", "USD")
            .context("payment_method", "credit_card")
            .severity(Severity::INFO)
            .emit();
        
        // Process payment
        auto result = process_payment(order_id, amount);
        
        // Log processing time metric (for anomaly detection)
        AGENTLOG_OBSERVE("payment.processing_time")
            .metric("latency_ms", result.processing_time_ms)
            .metric("amount_usd", amount)
            .entity("order_id", order_id)
            .emit();
        
        if (result.success) {
            // Success
            AGENTLOG_EVENT("payment.transaction.completed")
                .entity("order_id", order_id)
                .metric("amount_usd", amount)
                .metric("processing_time_ms", result.processing_time_ms)
                .context("status", "success")
                .severity(Severity::INFO)
                .emit();
        } else {
            // Failure - this is important!
            AGENTLOG_EVENT("payment.transaction.failed")
                .entity("order_id", order_id)
                .metric("amount_usd", amount)
                .metric("processing_time_ms", result.processing_time_ms)
                .context("error_code", result.error_code)
                .context("status", "failed")
                .severity(Severity::WARNING)
                .emit();
        }
        
        // Simulate anomaly: sudden processing spike every 50 transactions
        if (i % 50 == 0) {
            AGENTLOG_OBSERVE("payment.processing_time")
                .metric("latency_ms", 3000.0)  // 3 seconds!
                .metric("amount_usd", amount)
                .entity("order_id", order_id)
                .context("anomaly_type", "latency_spike")
                .emit();
            
            AGENTLOG_EVENT("payment.gateway.timeout")
                .entity("order_id", order_id)
                .entity("gateway", "stripe")
                .severity(Severity::ERROR)
                .message("Payment gateway timeout after 3000ms")
                .capture_stack_trace()
                .emit();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Display final statistics
    auto stats = Logger::instance().get_stats();
    std::cout << "\n=== Payment Service Statistics ===\n";
    std::cout << "Total events: " << stats.events_total << "\n";
    std::cout << "Anomalies detected: " << stats.anomalies_detected << "\n";
    std::cout << "Incidents created: " << stats.incidents_created << "\n";
    std::cout << "Events dropped: " << stats.events_dropped << "\n";
    
    global::info("Payment service shutting down");
    global::shutdown();
    
    return 0;
}
