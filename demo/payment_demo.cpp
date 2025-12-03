/**
 * Payment Service Demo - Full AgentLog Integration
 * Demonstrates AI-powered logging with anomaly detection, pattern recognition,
 * and automatic incident creation to Jira/PagerDuty/Slack simulators
 */

#include <agentlog/agentlog.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>

// ANSI color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

// Generate random transaction ID
std::string generate_txn_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    return "TXN-" + std::to_string(dis(gen));
}

// Generate random customer ID
std::string generate_customer_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 5);
    return "CUST-" + std::to_string(dis(gen));
}

// Simulate payment processing with various scenarios
void process_payment(int iteration) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> outcome_dis(1, 100);
    static std::uniform_int_distribution<> latency_dis(50, 500);
    static std::uniform_real_distribution<> amount_dis(10.0, 500.0);
    
    std::string txn_id = generate_txn_id();
    std::string customer_id = generate_customer_id();
    double amount = std::round(amount_dis(gen) * 100) / 100;
    int outcome = outcome_dis(gen);
    
    // Simulate different outcomes with probabilities
    if (outcome <= 5) {  // 5% - Fraud detection (HIGH anomaly)
        std::cout << COLOR_RED << "[" << iteration << "] " << txn_id 
                  << " - $" << amount << " - 🚫 FRAUD DETECTED" << COLOR_RESET << "\n";
        
        // This will trigger: Anomaly Detection + Pattern Recognition + Jira Ticket
        AGENTLOG_EVENT("payment.fraud_detected")
            .entity("transaction_id", txn_id)
            .entity("customer_id", customer_id)
            .metric("amount", amount)
            .metric("fraud_score", 0.95)
            .severity(agentlog::Severity::CRITICAL)
            .emit();
        
        std::cout << "  " << COLOR_MAGENTA << "🎫 AgentLog will create Jira ticket for fraud" << COLOR_RESET << "\n";
        
    } else if (outcome <= 15) {  // 10% - Timeout (triggers PagerDuty)
        int latency = 2000 + (outcome * 100);  // 2000-2500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(latency));
        
        std::cout << COLOR_RED << "[" << iteration << "] " << txn_id 
                  << " - $" << amount << " - 🔴 TIMEOUT (" << latency << "ms)" << COLOR_RESET << "\n";
        
        // High latency will trigger anomaly detection + PagerDuty alert
        AGENTLOG_OBSERVE("payment.latency")
            .metric("latency_ms", static_cast<double>(latency))
            .entity("transaction_id", txn_id)
            .context("endpoint", "/api/payment/process")
            .severity(agentlog::Severity::ERROR)
            .emit();
        
        std::cout << "  " << COLOR_MAGENTA << "🚨 AgentLog will trigger PagerDuty incident" << COLOR_RESET << "\n";
        
    } else if (outcome <= 25) {  // 10% - Insufficient funds (Slack notification)
        std::cout << COLOR_YELLOW << "[" << iteration << "] " << txn_id 
                  << " - $" << amount << " - ⚠️  INSUFFICIENT FUNDS" << COLOR_RESET << "\n";
        
        AGENTLOG_EVENT("payment.declined")
            .entity("transaction_id", txn_id)
            .entity("customer_id", customer_id)
            .entity("reason", "insufficient_funds")
            .metric("amount", amount)
            .severity(agentlog::Severity::WARNING)
            .emit();
        
        std::cout << "  " << COLOR_MAGENTA << "💬 AgentLog will send Slack notification" << COLOR_RESET << "\n";
        
    } else {  // 75% - Success
        int latency = latency_dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(latency));
        
        std::cout << COLOR_GREEN << "[" << iteration << "] " << txn_id 
                  << " - $" << amount << " - ✓ SUCCESS (latency: " << latency << "ms)" << COLOR_RESET << "\n";
        
        // Normal metrics - AgentLog learns this as baseline behavior
        AGENTLOG_OBSERVE("payment.latency")
            .metric("latency_ms", static_cast<double>(latency))
            .entity("transaction_id", txn_id)
            .context("endpoint", "/api/payment/process")
            .severity(agentlog::Severity::INFO)
            .emit();
        
        AGENTLOG_EVENT("payment.success")
            .entity("transaction_id", txn_id)
            .entity("customer_id", customer_id)
            .metric("amount", amount)
            .severity(agentlog::Severity::INFO)
            .emit();
    }
}

int main() {
    std::cout << COLOR_BOLD << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Payment Service Demo - AgentLog AI-Powered Logging     ║\n";
    std::cout << "║  Features: Anomaly Detection, Pattern Recognition       ║\n";
    std::cout << "║  Integrations: Jira, PagerDuty, Slack Simulators        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << COLOR_RESET << "\n\n";
    
    // Initialize AgentLog with AI features enabled
    agentlog::Config config;
    config.service_name = "payment-service";
    config.environment = "demo";
    config.enable_anomaly_detection = true;
    config.enable_pattern_matching = true;
    config.enable_correlation = true;
    config.enable_auto_incidents = true;
    config.incident_anomaly_threshold = 0.75;  // Create incidents for anomalies > 0.75
    
    // Enable file logging
    config.log_file_path = "payment_demo.log";
    config.log_to_console = true;  // Keep console output too
    
    // Enable external integrations (simulators)
    config.jira.enabled = true;
    config.jira.url = "http://localhost:8080";
    config.jira.project_key = "AGENT";
    
    config.pagerduty.enabled = true;
    config.pagerduty.integration_key = "payment-service-key";
    
    config.slack.enabled = true;
    config.slack.webhook_url = "http://localhost:8082/services/T00000000/B00000000/agentlog";
    config.slack.channel = "#agentlog-alerts";
    
    std::cout << COLOR_CYAN << "✓ AgentLog initialized with AI features:" << COLOR_RESET << "\n";
    std::cout << "  • Anomaly Detection: " << COLOR_GREEN << "ENABLED" << COLOR_RESET << "\n";
    std::cout << "  • Pattern Recognition: " << COLOR_GREEN << "ENABLED" << COLOR_RESET << "\n";
    std::cout << "  • Correlation Engine: " << COLOR_GREEN << "ENABLED" << COLOR_RESET << "\n";
    std::cout << "  • Auto Incidents: " << COLOR_GREEN << "ENABLED" << COLOR_RESET << " (threshold: 0.75)\n\n";
    
    std::cout << COLOR_CYAN << "✓ Connected to simulators:" << COLOR_RESET << "\n";
    std::cout << "  - Jira: http://localhost:8080/rest/api/2/issue\n";
    std::cout << "  - PagerDuty: http://localhost:8081/v2/enqueue\n";
    std::cout << "  - Slack: http://localhost:8082\n";
    std::cout << "  - Dashboard: http://localhost:3000\n\n";
    
    try {
        agentlog::global::init(config);
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "Failed to initialize AgentLog: " << e.what() << COLOR_RESET << "\n";
        return 1;
    }
    
    std::cout << COLOR_BOLD << "Processing payments (Press Ctrl+C to stop)...\n\n" << COLOR_RESET;
    
    // Process 100 payment transactions
    for (int i = 1; i <= 100; ++i) {
        process_payment(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Show summary every 20 transactions
        if (i % 20 == 0) {
            std::cout << "\n" << COLOR_BOLD << COLOR_CYAN << "📊 SUMMARY (after " << i << " transactions):" << COLOR_RESET << "\n";
            std::cout << "   " << COLOR_GREEN << "✅ Success: ~" << (i * 75 / 100) << " (75%)" << COLOR_RESET << "\n";
            std::cout << "   " << COLOR_RED << "🚫 Fraud: ~" << (i * 5 / 100) << " (5%)" << COLOR_RESET << "\n";
            std::cout << "   " << COLOR_RED << "🔴 Timeouts: ~" << (i * 10 / 100) << " (10%)" << COLOR_RESET << "\n";
            std::cout << "   " << COLOR_YELLOW << "⚠️  Declined: ~" << (i * 10 / 100) << " (10%)" << COLOR_RESET << "\n\n";
        }
    }
    
    std::cout << "\n" << COLOR_BOLD << COLOR_GREEN << "✓ Demo completed!" << COLOR_RESET << "\n\n";
    std::cout << COLOR_CYAN << "📊 View results in your browser:" << COLOR_RESET << "\n";
    std::cout << "  • Jira Tickets:        http://localhost:8080\n";
    std::cout << "  • PagerDuty Incidents: http://localhost:8081\n";
    std::cout << "  • Slack Messages:      http://localhost:8082\n";
    std::cout << "  • Dashboard:           http://localhost:3000\n\n";
    
    // Get statistics from AgentLog
    auto stats = agentlog::Logger::instance().get_stats();
    std::cout << COLOR_BOLD << "AgentLog Statistics:" << COLOR_RESET << "\n";
    std::cout << "  Total Events: " << stats.events_total << "\n";
    std::cout << "  Anomalies Detected: " << stats.anomalies_detected << "\n";
    std::cout << "  Incidents Created: " << stats.incidents_created << "\n\n";
    
    agentlog::global::shutdown();
    return 0;
}
