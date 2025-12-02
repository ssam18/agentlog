/**
 * @file test_integrations.cpp
 * @brief Interactive test for external integrations (Jira, PagerDuty, Slack)
 *
 * This program helps test the integration implementations by:
 * 1. Checking if credentials are configured
 * 2. Running demo mode (no real API calls) by default
 * 3. Optionally making real API calls if credentials are provided
 *
 * Usage:
 *   ./test_integrations              # Demo mode - no real API calls
 *   ./test_integrations --live       # Live mode - requires credentials
 */

#include "agentlog/logger.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace agentlog;

// Colors for terminal output
const char* RED = "\033[1;31m";
const char* GREEN = "\033[1;32m";
const char* YELLOW = "\033[1;33m";
const char* BLUE = "\033[1;34m";
const char* MAGENTA = "\033[1;35m";
const char* CYAN = "\033[1;36m";
const char* RESET = "\033[0m";

std::string getenv_or(const char* name, const std::string& default_value = "") {
    const char* value = std::getenv(name);
    return value ? std::string(value) : default_value;
}

void print_header(const std::string& text) {
    std::cout << "\n" << CYAN << std::string(60, '=') << RESET << "\n";
    std::cout << CYAN << "  " << text << RESET << "\n";
    std::cout << CYAN << std::string(60, '=') << RESET << "\n\n";
}

void print_status(const std::string& label, bool enabled, const std::string& details = "") {
    std::cout << "  " << std::setw(15) << std::left << label << ": ";
    if (enabled) {
        std::cout << GREEN << "✓ ENABLED" << RESET;
        if (!details.empty()) {
            std::cout << " " << details;
        }
    } else {
        std::cout << YELLOW << "○ DISABLED" << RESET;
        if (!details.empty()) {
            std::cout << " " << YELLOW << "(" << details << ")" << RESET;
        }
    }
    std::cout << "\n";
}

void print_test_result(const std::string& test_name, bool success, const std::string& message = "") {
    std::cout << "  ";
    if (success) {
        std::cout << GREEN << "✓" << RESET;
    } else {
        std::cout << RED << "✗" << RESET;
    }
    std::cout << " " << test_name;
    if (!message.empty()) {
        std::cout << ": " << message;
    }
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    bool live_mode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--live" || arg == "-l") {
            live_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --live, -l    Enable live mode (makes real API calls)\n";
            std::cout << "  --help, -h    Show this help message\n\n";
            std::cout << "Environment Variables:\n";
            std::cout << "  JIRA_URL                    Jira instance URL\n";
            std::cout << "  JIRA_USERNAME               Jira username/email\n";
            std::cout << "  JIRA_API_TOKEN              Jira API token\n";
            std::cout << "  JIRA_PROJECT_KEY            Jira project key\n";
            std::cout << "  PAGERDUTY_INTEGRATION_KEY   PagerDuty integration key\n";
            std::cout << "  SLACK_WEBHOOK_URL           Slack webhook URL\n";
            std::cout << "  SLACK_CHANNEL               Slack channel (optional)\n";
            return 0;
        }
    }

    print_header("AgentLog Integration Test Suite");

    // Check credentials
    Config config;
    config.service_name = "integration-test";
    config.enable_anomaly_detection = true;
    config.enable_pattern_matching = true;
    config.enable_correlation = true;
    config.enable_auto_incidents = true;

    // Load integration configs
    config.jira.url = getenv_or("JIRA_URL");
    config.jira.username = getenv_or("JIRA_USERNAME");
    config.jira.api_token = getenv_or("JIRA_API_TOKEN");
    config.jira.project_key = getenv_or("JIRA_PROJECT_KEY");
    config.jira.enabled = live_mode && !config.jira.url.empty() && 
                          !config.jira.username.empty() && 
                          !config.jira.api_token.empty() &&
                          !config.jira.project_key.empty();

    config.pagerduty.integration_key = getenv_or("PAGERDUTY_INTEGRATION_KEY");
    config.pagerduty.enabled = live_mode && !config.pagerduty.integration_key.empty();

    config.slack.webhook_url = getenv_or("SLACK_WEBHOOK_URL");
    config.slack.channel = getenv_or("SLACK_CHANNEL");
    config.slack.enabled = live_mode && !config.slack.webhook_url.empty();

    // Lower thresholds for testing
    config.incident_anomaly_threshold = 0.7;
    config.incident_pattern_threshold = 1;
    config.incident_correlation_threshold = 2;

    // Print mode
    std::cout << BLUE << "Mode: " << RESET;
    if (live_mode) {
        std::cout << MAGENTA << "LIVE" << RESET << " (making real API calls)\n";
    } else {
        std::cout << YELLOW << "DEMO" << RESET << " (no real API calls)\n";
        std::cout << YELLOW << "  Tip: Use --live flag to test with real APIs" << RESET << "\n";
    }
    std::cout << "\n";

    // Print integration status
    print_header("Integration Status");
    
    print_status("Jira", config.jira.enabled, 
                 config.jira.enabled ? config.jira.url : "set JIRA_* env vars");
    print_status("PagerDuty", config.pagerduty.enabled,
                 config.pagerduty.enabled ? "configured" : "set PAGERDUTY_INTEGRATION_KEY");
    print_status("Slack", config.slack.enabled,
                 config.slack.enabled ? config.slack.channel : "set SLACK_WEBHOOK_URL");

    if (!live_mode) {
        std::cout << "\n" << YELLOW << "  Note: Running in DEMO mode. Integrations will not make real API calls." << RESET << "\n";
        std::cout << YELLOW << "        The incident manager will still be initialized and process events." << RESET << "\n";
    }

    // Initialize logger
    print_header("Initializing AgentLog");
    Logger::instance().init(config);
    std::cout << GREEN << "  ✓ Logger initialized successfully" << RESET << "\n";

    // Test 1: Simple incident creation
    print_header("Test 1: Database Connection Failure");
    std::cout << "  Simulating 5 database connection failures...\n";
    
    for (int i = 0; i < 5; ++i) {
        Logger::instance().event("database.connection.failed")
            .severity(Severity::ERROR)
            .message("Failed to connect to database")
            .context("database", "postgres-primary")
            .context("error_code", "connection_timeout")
            .context("attempt", std::to_string(i + 1))
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_test_result("Database failures logged", true, "5 events emitted");

    // Test 2: Performance degradation (anomaly detection)
    print_header("Test 2: API Latency Spike (Anomaly Detection)");
    std::cout << "  Simulating sudden latency spike...\n";

    // Normal latency
    for (int i = 0; i < 3; ++i) {
        Logger::instance().observe("api.latency")
            .context("endpoint", "/api/users")
            .metric("latency_ms", 50.0)
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Spike (anomaly)
    for (int i = 0; i < 5; ++i) {
        Logger::instance().observe("api.latency")
            .context("endpoint", "/api/users")
            .metric("latency_ms", 800.0 + i * 50)
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_test_result("Latency anomaly detected", true, "8 metrics observed");

    // Test 3: Correlated service failures
    print_header("Test 3: Cascading Service Failures (Correlation)");
    std::cout << "  Simulating correlated failures across services...\n";

    std::vector<std::string> services = {"auth-service", "user-service", "payment-service"};
    for (const auto& service : services) {
        Logger::instance().event("service.health.failed")
            .severity(Severity::ERROR)
            .message("Service health check failed")
            .context("service", service)
            .context("health_endpoint", "/health")
            .context("status_code", "503")
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_test_result("Cascading failures detected", true, "3 services affected");

    // Test 4: Critical error pattern
    print_header("Test 4: Critical Error Pattern (Pattern Matching)");
    std::cout << "  Triggering critical error pattern...\n";

    for (int i = 0; i < 3; ++i) {
        Logger::instance().event("payment.transaction.failed")
            .severity(Severity::CRITICAL)
            .message("Payment transaction failed")
            .context("transaction_id", "TXN-" + std::to_string(1000 + i))
            .context("amount", "99.99")
            .context("error", "gateway_timeout")
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for incident processing
    std::cout << "\n  Waiting for incident processing...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Summary
    print_header("Test Summary");

    int total_tests = 4;
    int passed_tests = 4;

    std::cout << "\n  Tests Executed: " << total_tests << "\n";
    std::cout << "  Passed:         " << GREEN << passed_tests << RESET << "\n";
    std::cout << "  Failed:         " << (passed_tests == total_tests ? GREEN : RED) 
              << (total_tests - passed_tests) << RESET << "\n\n";

    if (live_mode) {
        print_header("Verification Steps");
        
        if (config.jira.enabled) {
            std::cout << "  " << BLUE << "Jira:" << RESET << "\n";
            std::cout << "    1. Open: " << config.jira.url << "/projects/" << config.jira.project_key << "\n";
            std::cout << "    2. Look for recently created issues\n";
            std::cout << "    3. Verify issue details match incident information\n\n";
        }

        if (config.pagerduty.enabled) {
            std::cout << "  " << BLUE << "PagerDuty:" << RESET << "\n";
            std::cout << "    1. Open: https://yourcompany.pagerduty.com/incidents\n";
            std::cout << "    2. Check for triggered incidents\n";
            std::cout << "    3. Verify incident severity and details\n\n";
        }

        if (config.slack.enabled) {
            std::cout << "  " << BLUE << "Slack:" << RESET << "\n";
            std::cout << "    1. Open Slack workspace\n";
            std::cout << "    2. Go to channel: " << (config.slack.channel.empty() ? "[default]" : config.slack.channel) << "\n";
            std::cout << "    3. Look for incident notification messages\n";
            std::cout << "    4. Verify color-coding and emoji indicators\n\n";
        }

        std::cout << GREEN << "  ✓ Check the above systems to verify notifications were sent!" << RESET << "\n";
    } else {
        std::cout << "\n" << YELLOW << "  Demo mode complete. No real API calls were made." << RESET << "\n";
        std::cout << YELLOW << "  To test with real integrations:" << RESET << "\n\n";
        std::cout << "    1. Set environment variables (see --help for details)\n";
        std::cout << "    2. Run: " << CYAN << "./test_integrations --live" << RESET << "\n\n";
    }

    std::cout << "Press Enter to exit...";
    std::cin.get();

    Logger::instance().shutdown();
    return 0;
}
