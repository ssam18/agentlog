/**
 * Simple Payment Service Simulator for AgentLog Demo
 * Tests webhook integration with Jira, PagerDuty, and Slack simulators
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <curl/curl.h>
#include <sstream>

// Webhook URLs (matching docker-compose service names)
const std::string JIRA_URL = "http://localhost:8080/rest/api/2/issue";
const std::string PAGERDUTY_URL = "http://localhost:8081/v2/enqueue";
const std::string SLACK_URL = "http://localhost:8082/services/T00000000/B00000000/agentlog";

// Helper to send HTTP POST
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool send_webhook(const std::string& url, const std::string& json_data) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK);
}

// Generate transaction ID
std::string generate_txn_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    return "TXN-" + std::to_string(dis(gen));
}

// Create Jira ticket for fraud
void create_jira_ticket(const std::string& txn_id, const std::string& customer_id, double amount) {
    std::ostringstream json;
    json << "{"
         << "\"fields\":{"
         << "\"project\":{\"key\":\"AGENT\"},"
         << "\"summary\":\"Fraud detected - " << txn_id << "\","
         << "\"description\":\"Suspicious transaction blocked. Customer: " << customer_id 
         << ", Amount: $" << amount << ", Fraud score: 0.95\","
         << "\"issuetype\":{\"name\":\"Bug\"},"
         << "\"priority\":{\"name\":\"Critical\"},"
         << "\"labels\":[\"agentlog\",\"fraud-detection\",\"payment-service\"]"
         << "}}";
    
    if (send_webhook(JIRA_URL, json.str())) {
        std::cout << "  🎫 Jira ticket created for " << txn_id << "\n";
    }
}

// Trigger PagerDuty incident for high latency
void trigger_pagerduty(const std::string& txn_id, int latency_ms) {
    std::ostringstream json;
    json << "{"
         << "\"routing_key\":\"payment-service-key\","
         << "\"event\":{"
         << "\"event_action\":\"trigger\","
         << "\"dedup_key\":\"payment-latency-" << txn_id << "\","
         << "\"payload\":{"
         << "\"summary\":\"High latency in payment processing\","
         << "\"severity\":\"critical\","
         << "\"source\":\"payment-service\","
         << "\"component\":\"payment-gateway\","
         << "\"custom_details\":{"
         << "\"transaction_id\":\"" << txn_id << "\","
         << "\"latency_ms\":\"" << latency_ms << "\","
         << "\"threshold_ms\":\"500\""
         << "}}}}";
    
    if (send_webhook(PAGERDUTY_URL, json.str())) {
        std::cout << "  🚨 PagerDuty alert sent for " << txn_id << " (" << latency_ms << "ms)\n";
    }
}

// Send Slack notification
void send_slack_notification(const std::string& message, const std::string& color, const std::string& title, const std::string& details) {
    std::ostringstream json;
    json << "{"
         << "\"text\":\"" << message << "\","
         << "\"channel\":\"#agentlog-alerts\","
         << "\"username\":\"AgentLog Bot\","
         << "\"icon_emoji\":\":robot_face:\","
         << "\"attachments\":[{"
         << "\"color\":\"" << color << "\","
         << "\"title\":\"" << title << "\","
         << "\"text\":\"" << details << "\","
         << "\"footer\":\"AgentLog Demo\""
         << "}]}";
    
    if (send_webhook(SLACK_URL, json.str())) {
        std::cout << "  💬 Slack notification sent\n";
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Payment Service Demo - AgentLog Integration          ║\n";
    std::cout << "║     Testing Jira, PagerDuty, and Slack Simulators        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "✓ Connected to simulators:\n";
    std::cout << "  - Jira: " << JIRA_URL << "\n";
    std::cout << "  - PagerDuty: " << PAGERDUTY_URL << "\n";
    std::cout << "  - Slack: " << SLACK_URL << "\n";
    std::cout << "  - Dashboard: http://localhost:3000\n\n";
    std::cout << "Processing payments (Press Ctrl+C to stop)...\n\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> outcome_dis(1, 100);
    std::uniform_int_distribution<> latency_dis(50, 500);
    std::uniform_real_distribution<> amount_dis(10.0, 500.0);
    
    std::vector<std::string> customers = {"CUST-001", "CUST-002", "CUST-003", "CUST-004", "CUST-005"};
    std::uniform_int_distribution<> customer_dis(0, customers.size() - 1);
    
    int transaction_count = 0;
    int success_count = 0;
    int fraud_count = 0;
    int timeout_count = 0;
    int error_count = 0;
    
    while (true) {
        std::string txn_id = generate_txn_id();
        std::string customer_id = customers[customer_dis(gen)];
        double amount = std::round(amount_dis(gen) * 100.0) / 100.0;
        int outcome = outcome_dis(gen);
        int latency = latency_dis(gen);
        
        transaction_count++;
        
        std::cout << "[" << transaction_count << "] " << txn_id << " - $" << amount << " - ";
        
        if (outcome <= 70) {
            // 70% success
            std::cout << "✓ SUCCESS (latency: " << latency << "ms)\n";
            success_count++;
            
        } else if (outcome <= 85) {
            // 15% insufficient funds
            std::cout << "⚠️  INSUFFICIENT FUNDS\n";
            error_count++;
            send_slack_notification(
                "⚠️ Payment declined - insufficient funds",
                "warning",
                "Payment Declined",
                "Transaction " + txn_id + " declined: Customer " + customer_id + " has insufficient funds. Amount: $" + std::to_string(amount)
            );
            
        } else if (outcome <= 95) {
            // 10% gateway timeout - triggers PagerDuty
            int high_latency = 2000 + latency;
            std::cout << "🔴 TIMEOUT (" << high_latency << "ms) - PagerDuty alert sent\n";
            timeout_count++;
            
            trigger_pagerduty(txn_id, high_latency);
            send_slack_notification(
                "🔴 Critical: Payment gateway timeout",
                "danger",
                "Gateway Timeout Alert",
                "Transaction " + txn_id + " failed with " + std::to_string(high_latency) + "ms latency (threshold: 500ms)"
            );
            
        } else {
            // 5% fraud - creates Jira ticket
            std::cout << "🚫 FRAUD DETECTED - Jira ticket created\n";
            fraud_count++;
            
            create_jira_ticket(txn_id, customer_id, amount);
            trigger_pagerduty(txn_id + "-fraud", 0);
            send_slack_notification(
                "🚫 Fraud Alert: Suspicious transaction blocked",
                "danger",
                "Fraud Detection Alert",
                "Transaction " + txn_id + " blocked. Customer: " + customer_id + ", Amount: $" + std::to_string(amount) + ", Fraud score: 0.95"
            );
        }
        
        // Summary every 20 transactions
        if (transaction_count % 20 == 0) {
            std::cout << "\n📊 SUMMARY (after " << transaction_count << " transactions):\n";
            std::cout << "   ✅ Success: " << success_count << " (" << (success_count*100/transaction_count) << "%)\n";
            std::cout << "   🚫 Fraud: " << fraud_count << " (" << (fraud_count*100/transaction_count) << "%)\n";
            std::cout << "   🔴 Timeouts: " << timeout_count << " (" << (timeout_count*100/transaction_count) << "%)\n";
            std::cout << "   ⚠️  Other Errors: " << error_count << " (" << (error_count*100/transaction_count) << "%)\n\n";
            
            send_slack_notification(
                "📊 Payment Processing Summary",
                "good",
                "Status Report - " + std::to_string(transaction_count) + " transactions",
                "Success: " + std::to_string(success_count) + " | Fraud: " + std::to_string(fraud_count) + 
                " | Timeouts: " + std::to_string(timeout_count) + " | Errors: " + std::to_string(error_count)
            );
        }
        
        // Wait 1-2 seconds between transactions
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + (outcome % 1000)));
    }
    
    curl_global_cleanup();
    return 0;
}
