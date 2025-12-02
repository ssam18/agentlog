/**
 * @file integration_demo.cpp
 * @brief Demonstration of external integrations (Jira, PagerDuty, Slack)
 *
 * This example shows how to configure and use external integrations
 * to automatically create tickets, trigger incidents, and send notifications
 * when incidents are detected.
 *
 * Setup Instructions:
 * -------------------
 * 1. Jira Cloud:
 *    - Get your Atlassian site URL (e.g., https://yourcompany.atlassian.net)
 *    - Create an API token: https://id.atlassian.com/manage-profile/security/api-tokens
 *    - Note your project key (e.g., "PROJ")
 *
 * 2. PagerDuty:
 *    - Create a service in PagerDuty
 *    - Add "Events API V2" integration
 *    - Copy the Integration Key
 *
 * 3. Slack:
 *    - Create an Incoming Webhook: https://api.slack.com/messaging/webhooks
 *    - Copy the webhook URL (https://hooks.slack.com/services/...)
 *
 * Usage:
 * ------
 * Set environment variables before running:
 *   export JIRA_URL="https://yourcompany.atlassian.net"
 *   export JIRA_USERNAME="your.email@company.com"
 *   export JIRA_API_TOKEN="your-api-token"
 *   export JIRA_PROJECT_KEY="PROJ"
 *   export PAGERDUTY_INTEGRATION_KEY="your-integration-key"
 *   export SLACK_WEBHOOK_URL="https://hooks.slack.com/services/..."
 *
 * Then run: ./integration_demo
 */

#include "agentlog/logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace agentlog;

// Helper to get environment variable with fallback
std::string getenv_or(const char* name, const std::string& default_value = "") {
    const char* value = std::getenv(name);
    return value ? std::string(value) : default_value;
}

int main() {
    std::cout << "==================================\n";
    std::cout << "AgentLog External Integrations Demo\n";
    std::cout << "==================================\n\n";

    // Configure AgentLog with integrations
    Config config;
    config.service_name = "integration-demo-service";
    config.enable_anomaly_detection = true;
    config.enable_pattern_matching = true;
    config.enable_correlation = true;
    config.enable_auto_incidents = true;

    // Configure Jira integration
    config.jira.url = getenv_or("JIRA_URL");
    config.jira.username = getenv_or("JIRA_USERNAME");
    config.jira.api_token = getenv_or("JIRA_API_TOKEN");
    config.jira.project_key = getenv_or("JIRA_PROJECT_KEY");
    config.jira.enabled = !config.jira.url.empty() && 
                          !config.jira.username.empty() && 
                          !config.jira.api_token.empty();

    // Configure PagerDuty integration
    config.pagerduty.integration_key = getenv_or("PAGERDUTY_INTEGRATION_KEY");
    config.pagerduty.enabled = !config.pagerduty.integration_key.empty();

    // Configure Slack integration
    config.slack.webhook_url = getenv_or("SLACK_WEBHOOK_URL");
    config.slack.channel = getenv_or("SLACK_CHANNEL");  // Optional
    config.slack.enabled = !config.slack.webhook_url.empty();

    // Lower thresholds for demo purposes
    config.incident_anomaly_threshold = 0.7;
    config.incident_pattern_threshold = 1;
    config.incident_correlation_threshold = 2;

    // Initialize logger
    Logger::instance().init(config);

    std::cout << "\nConfiguration:\n";
    std::cout << "  Jira:        " << (config.jira.enabled ? "ENABLED (" + config.jira.url + ")" : "DISABLED (set JIRA_URL, JIRA_USERNAME, JIRA_API_TOKEN, JIRA_PROJECT_KEY)") << "\n";
    std::cout << "  PagerDuty:   " << (config.pagerduty.enabled ? "ENABLED" : "DISABLED (set PAGERDUTY_INTEGRATION_KEY)") << "\n";
    std::cout << "  Slack:       " << (config.slack.enabled ? "ENABLED" : "DISABLED (set SLACK_WEBHOOK_URL)") << "\n";
    std::cout << "\n";

    if (!config.jira.enabled && !config.pagerduty.enabled && !config.slack.enabled) {
        std::cout << "⚠️  No integrations configured. This demo will still work but won't send external notifications.\n";
        std::cout << "    See the file header for setup instructions.\n\n";
    }

    // Simulate a production incident scenario
    std::cout << "Simulating production incident scenario...\n\n";

    // 1. Normal operations
    Logger::instance().event("api.request")
        .context("endpoint", "/api/users")
        .context("method", "GET")
        .metric("response_time_ms", 45.0)
        .emit();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Performance degradation detected
    for (int i = 0; i < 5; ++i) {
        Logger::instance().observe("api.latency")
            .context("endpoint", "/api/users")
            .metric("latency_ms", 400.0 + i * 50)
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // 3. Database connection failures (correlated events)
    for (int i = 0; i < 3; ++i) {
        Logger::instance().event("database.connection.failed")
            .severity(Severity::ERROR)
            .message("Database connection failed")
            .context("database", "postgres-primary")
            .context("error_code", "connection_timeout")
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 4. Service errors (pattern match)
    for (int i = 0; i < 3; ++i) {
        Logger::instance().event("service.health.failed")
            .severity(Severity::ERROR)
            .message("Service health check failed")
            .context("service", "user-service")
            .context("health_endpoint", "/health")
            .emit();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait a bit for incident processing
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\n\n==================================\n";
    std::cout << "Demo Complete!\n";
    std::cout << "==================================\n\n";

    if (config.jira.enabled) {
        std::cout << "✅ Check your Jira project for new tickets!\n";
        std::cout << "   URL: " << config.jira.url << "/projects/" << config.jira.project_key << "\n";
    }
    
    if (config.pagerduty.enabled) {
        std::cout << "✅ Check your PagerDuty dashboard for new incidents!\n";
        std::cout << "   URL: https://yourcompany.pagerduty.com/incidents\n";
    }
    
    if (config.slack.enabled) {
        std::cout << "✅ Check your Slack channel for incident notifications!\n";
        if (!config.slack.channel.empty()) {
            std::cout << "   Channel: " << config.slack.channel << "\n";
        }
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    Logger::instance().shutdown();
    return 0;
}
