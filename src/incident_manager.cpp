// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/incident_manager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Include curl helper
#include "curl_helper.h"
#include <sstream>
#include <cstring>
#include <cstdlib>

namespace agentlog {

// JSON string escaping helper
static std::string json_escape(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c < 32) {
                    escaped << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                } else {
                    escaped << c;
                }
        }
    }
    return escaped.str();
}

// Simple Base64 encoding helper
static std::string base64_encode(const std::string& input) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string output;
    int val = 0;
    int valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (output.size() % 4) {
        output.push_back('=');
    }
    
    return output;
}

//=============================================================================
// Incident Implementation
//=============================================================================

std::string Incident::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"incident_id\": \"" << incident_id << "\",\n";
    oss << "  \"title\": \"" << title << "\",\n";
    oss << "  \"description\": \"" << description << "\",\n";
    oss << "  \"severity\": \"" << incident_severity_to_string(severity) << "\",\n";
    oss << "  \"status\": \"" << static_cast<int>(status) << "\",\n";
    oss << "  \"anomaly_score\": " << anomaly_score << ",\n";
    oss << "  \"affected_services\": " << affected_services_count << ",\n";
    oss << "  \"affected_users\": " << affected_users_count << ",\n";
    oss << "  \"event_count\": " << event_ids.size() << ",\n";
    
    if (root_cause) {
        oss << "  \"root_cause\": \"" << *root_cause << "\",\n";
    }
    
    if (jira_ticket_id) {
        oss << "  \"jira_ticket\": \"" << *jira_ticket_id << "\",\n";
    }
    
    if (pagerduty_incident_id) {
        oss << "  \"pagerduty_incident\": \"" << *pagerduty_incident_id << "\",\n";
    }
    
    auto time_t = std::chrono::system_clock::to_time_t(created_at);
    oss << "  \"created_at\": \"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\"\n";
    oss << "}";
    
    return oss.str();
}

std::string Incident::summary() const {
    std::ostringstream oss;
    oss << "[" << incident_severity_to_string(severity) << "] " << title;
    oss << " (ID: " << incident_id << ", Score: " << anomaly_score << ")";
    return oss.str();
}

//=============================================================================
// IncidentManager Implementation
//=============================================================================

std::optional<Incident> IncidentManager::evaluate_event(
    const LogEvent& event,
    const std::vector<Correlation>& correlations,
    const std::vector<std::string>& matched_patterns) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if event meets thresholds
    bool should_create = false;
    
    if (event.anomaly_score() >= config_.anomaly_threshold) {
        should_create = true;
    }
    
    if (matched_patterns.size() >= config_.pattern_match_threshold) {
        should_create = true;
    }
    
    if (correlations.size() >= config_.correlated_events_threshold) {
        should_create = true;
    }
    
    if (!should_create) {
        return std::nullopt;
    }
    
    // Create incident
    Incident incident;
    incident.incident_id = generate_incident_id();
    incident.created_at = std::chrono::system_clock::now();
    incident.status = IncidentStatus::OPEN;
    incident.anomaly_score = event.anomaly_score();
    
    // Calculate severity
    incident.severity = calculate_severity(
        event.anomaly_score(),
        matched_patterns.size(),
        correlations.size()
    );
    
    // Build title
    std::ostringstream title_ss;
    if (!matched_patterns.empty()) {
        title_ss << "Pattern detected: " << matched_patterns.front();
    } else {
        title_ss << "Anomaly in " << event.event_type();
    }
    incident.title = title_ss.str();
    
    // Build description
    std::ostringstream desc_ss;
    desc_ss << "Incident created from event: " << event.event_type() << "\n";
    desc_ss << "Anomaly score: " << event.anomaly_score() << "\n";
    
    if (!matched_patterns.empty()) {
        desc_ss << "\nMatched patterns:\n";
        for (const auto& pattern : matched_patterns) {
            desc_ss << "  - " << pattern << "\n";
        }
    }
    
    if (!correlations.empty()) {
        desc_ss << "\nCorrelated events: " << correlations.size() << "\n";
        for (const auto& corr : correlations) {
            desc_ss << "  - " << corr.reason << " (confidence: " << corr.confidence << ")\n";
        }
    }
    
    if (!event.message().empty()) {
        desc_ss << "\nMessage: " << event.message() << "\n";
    }
    
    if (!event.entities().empty()) {
        desc_ss << "\nEntities:\n";
        for (const auto& [key, value] : event.entities()) {
            desc_ss << "  " << key << ": " << value << "\n";
        }
    }
    
    if (!event.metrics().empty()) {
        desc_ss << "\nMetrics:\n";
        for (const auto& [key, value] : event.metrics()) {
            desc_ss << "  " << key << ": " << value << "\n";
        }
    }
    
    incident.description = desc_ss.str();
    
    // Add event IDs
    incident.event_ids.push_back(event.event_id());
    for (const auto& corr : correlations) {
        incident.event_ids.insert(incident.event_ids.end(),
                                 corr.event_ids.begin(),
                                 corr.event_ids.end());
    }
    
    // Count affected services
    std::unordered_set<std::string> services;
    if (!event.service_name().empty()) {
        services.insert(event.service_name());
    }
    incident.affected_services_count = services.size();
    
    // Add labels
    incident.labels["severity"] = incident_severity_to_string(incident.severity);
    incident.labels["event_type"] = event.event_type();
    if (!event.service_name().empty()) {
        incident.labels["service"] = event.service_name();
    }
    
    // Add tags
    if (event.anomaly_score() >= 0.9) {
        incident.tags.push_back("critical-anomaly");
    }
    for (const auto& pattern : matched_patterns) {
        incident.tags.push_back("pattern:" + pattern);
    }
    
    // Check for duplicates
    if (config_.enable_deduplication) {
        if (auto dup_id = find_duplicate(incident)) {
            // Found duplicate, don't create new incident
            stats_.deduplicated++;
            return std::nullopt;
        }
    }
    
    // Store incident
    incidents_[incident.incident_id] = incident;
    stats_.total_created++;
    stats_.currently_open++;
    
    // Notify external integrations
    for (const auto& integration : integrations_) {
        try {
            std::string external_id = integration->create_incident(incident);
            // Store external ID in the incident based on integration type
            // (You could extend Incident struct to store multiple external IDs)
        } catch (const std::exception& e) {
            std::cerr << "Integration error: " << e.what() << std::endl;
        }
    }
    
    // Trigger callbacks
    for (const auto& callback : on_created_callbacks_) {
        callback(incident);
    }
    
    return incident;
}

Incident IncidentManager::create_incident(
    const std::string& title,
    const std::string& description,
    IncidentSeverity severity,
    const std::vector<uint64_t>& event_ids) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    Incident incident;
    incident.incident_id = generate_incident_id();
    incident.title = title;
    incident.description = description;
    incident.severity = severity;
    incident.status = IncidentStatus::OPEN;
    incident.created_at = std::chrono::system_clock::now();
    incident.event_ids = event_ids;
    incident.anomaly_score = 0.0;
    incident.affected_services_count = 0;
    incident.affected_users_count = 0;
    
    incidents_[incident.incident_id] = incident;
    stats_.total_created++;
    stats_.currently_open++;
    
    // Notify external integrations
    for (const auto& integration : integrations_) {
        try {
            std::string external_id = integration->create_incident(incident);
            // Store external ID in the incident based on integration type
            // (You could extend Incident struct to store multiple external IDs)
        } catch (const std::exception& e) {
            std::cerr << "Integration error: " << e.what() << std::endl;
        }
    }
    
    for (const auto& callback : on_created_callbacks_) {
        callback(incident);
    }
    
    return incident;
}

void IncidentManager::update_status(const std::string& incident_id, IncidentStatus new_status) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = incidents_.find(incident_id);
    if (it != incidents_.end()) {
        it->second.status = new_status;
    }
}

void IncidentManager::resolve_incident(const std::string& incident_id, const std::string& resolution) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = incidents_.find(incident_id);
    if (it != incidents_.end()) {
        it->second.status = IncidentStatus::RESOLVED;
        it->second.resolved_at = std::chrono::system_clock::now();
        it->second.root_cause = resolution;
        
        stats_.currently_open--;
        stats_.resolved++;
        
        // Notify external integrations
        for (const auto& integration : integrations_) {
            try {
                // For Jira: external_id would be ticket key
                // For PagerDuty: external_id would be dedup_key
                // For Slack: external_id would be "SLACK-{incident_id}"
                if (it->second.jira_ticket_id) {
                    integration->resolve_incident(*it->second.jira_ticket_id, resolution);
                }
                if (it->second.pagerduty_incident_id) {
                    integration->resolve_incident(*it->second.pagerduty_incident_id, resolution);
                }
                // Slack integration will be called with "SLACK-{incident_id}" format
                integration->resolve_incident("SLACK-" + incident_id, resolution);
            } catch (const std::exception& e) {
                std::cerr << "Integration error on resolve: " << e.what() << std::endl;
            }
        }
        
        for (const auto& callback : on_resolved_callbacks_) {
            callback(it->second);
        }
    }
}

std::optional<Incident> IncidentManager::get_incident(const std::string& incident_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = incidents_.find(incident_id);
    if (it != incidents_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<Incident> IncidentManager::get_open_incidents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Incident> result;
    for (const auto& [id, incident] : incidents_) {
        if (incident.status == IncidentStatus::OPEN ||
            incident.status == IncidentStatus::INVESTIGATING) {
            result.push_back(incident);
        }
    }
    
    return result;
}

std::vector<Incident> IncidentManager::get_all_incidents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Incident> result;
    for (const auto& [id, incident] : incidents_) {
        result.push_back(incident);
    }
    
    return result;
}

std::optional<std::string> IncidentManager::find_duplicate(const Incident& incident) const {
    auto cutoff = incident.created_at - config_.deduplication_window;
    
    for (const auto& [id, existing] : incidents_) {
        if (existing.created_at < cutoff) {
            continue;
        }
        
        if (existing.status == IncidentStatus::RESOLVED ||
            existing.status == IncidentStatus::CLOSED) {
            continue;
        }
        
        // Check if similar
        if (existing.title == incident.title &&
            existing.severity == incident.severity) {
            return id;
        }
        
        // Check if event IDs overlap significantly
        std::unordered_set<uint64_t> existing_set(
            existing.event_ids.begin(), existing.event_ids.end());
        
        size_t overlap = 0;
        for (uint64_t id : incident.event_ids) {
            if (existing_set.find(id) != existing_set.end()) {
                overlap++;
            }
        }
        
        if (overlap > incident.event_ids.size() / 2) {
            return id;
        }
    }
    
    return std::nullopt;
}

void IncidentManager::auto_resolve_stale_incidents() {
    if (!config_.enable_auto_resolution) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - config_.resolution_timeout;
    
    for (auto& [id, incident] : incidents_) {
        if (incident.status == IncidentStatus::OPEN ||
            incident.status == IncidentStatus::INVESTIGATING) {
            if (incident.created_at < cutoff) {
                incident.status = IncidentStatus::RESOLVED;
                incident.resolved_at = std::chrono::system_clock::now();
                incident.root_cause = "Auto-resolved: no further activity";
                
                stats_.currently_open--;
                stats_.resolved++;
            }
        }
    }
}

void IncidentManager::register_integration(std::shared_ptr<IncidentIntegration> integration) {
    std::lock_guard<std::mutex> lock(mutex_);
    integrations_.push_back(integration);
}

void IncidentManager::on_incident_created(IncidentCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_created_callbacks_.push_back(std::move(callback));
}

void IncidentManager::on_incident_resolved(IncidentCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_resolved_callbacks_.push_back(std::move(callback));
}

IncidentManager::Stats IncidentManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

IncidentSeverity IncidentManager::calculate_severity(
    double anomaly_score,
    size_t pattern_matches,
    size_t correlated_events) const {
    
    if (anomaly_score >= config_.critical_threshold) {
        return IncidentSeverity::CRITICAL;
    }
    
    if (anomaly_score >= config_.high_threshold || pattern_matches >= 2) {
        return IncidentSeverity::HIGH;
    }
    
    if (anomaly_score >= config_.medium_threshold || correlated_events >= 5) {
        return IncidentSeverity::MEDIUM;
    }
    
    return IncidentSeverity::LOW;
}

std::string IncidentManager::generate_incident_id() {
    uint64_t id = next_incident_id_.fetch_add(1);
    
    std::ostringstream oss;
    oss << "INC-" << std::setw(6) << std::setfill('0') << id;
    
    return oss.str();
}

//=============================================================================
// Integration Implementations
//=============================================================================

// Helper to convert severity to Jira priority
static std::string severity_to_jira_priority(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::CRITICAL: return "Highest";
        case IncidentSeverity::HIGH: return "High";
        case IncidentSeverity::MEDIUM: return "Medium";
        case IncidentSeverity::LOW: return "Low";
        default: return "Medium";
    }
}

std::string JiraIntegration::create_incident(const Incident& incident) {
    if (config_.url.empty() || config_.project_key.empty()) {
        return "JIRA-DISABLED";
    }
    
    try {
        // Build JSON payload
        std::ostringstream json;
        json << "{"
             << "\"fields\": {"
             << "\"project\": {\"key\": \"" << config_.project_key << "\"},"
             << "\"summary\": \"" << json_escape(incident.title) << "\","
             << "\"description\": \"" << json_escape(incident.description) << "\","
             << "\"issuetype\": {\"name\": \"Bug\"},"
             << "\"priority\": {\"name\": \"" << severity_to_jira_priority(incident.severity) << "\"}"  
             << "}}";
        
        // Set authentication header
        std::string auth = config_.username + ":" + config_.api_token;
        std::string auth_encoded = base64_encode(auth);
        
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Basic " + auth_encoded;
        
        // Make POST request
        std::string url = config_.url;
        if (url.back() == '/') url.pop_back();
        url += "/rest/api/2/issue";
        
        CurlHelper curl;
        auto response = curl.post(url, json.str(), headers);
        
        if (response.success && response.status_code == 201) {
            // Parse response to get issue key
            std::string body = response.body;
            size_t key_pos = body.find("\"key\":\"");
            if (key_pos != std::string::npos) {
                key_pos += 7;
                size_t end_pos = body.find("\"", key_pos);
                return body.substr(key_pos, end_pos - key_pos);
            }
            return "JIRA-CREATED";
        } else {
            std::cerr << "Jira API error: " << response.status_code << " - " << response.body << "\n";
            return "JIRA-ERROR";
        }
    } catch (const std::exception& e) {
        std::cerr << "Jira integration exception: " << e.what() << "\n";
        return "JIRA-EXCEPTION";
    }
}

void JiraIntegration::update_incident(const std::string& external_id, const Incident& incident) {
    // Note: libcurl doesn't have a simple PUT method in our helper, 
    // and updates are less critical for now. Can be implemented later if needed.
    (void)external_id;
    (void)incident;
}

void JiraIntegration::resolve_incident(const std::string& external_id, const std::string& resolution) {
    if (config_.url.empty() || external_id.find("JIRA-") != 0) {
        return;
    }
    
    try {
        // Transition to "Done" status (transition ID may vary)
        std::ostringstream json;
        json << "{"
             << "\"transition\": {\"id\": \"31\"},"  // 31 is common for "Done"
             << "\"fields\": {\"resolution\": {\"name\": \"" << resolution << "\"}}"
             << "}";
        
        std::string auth = config_.username + ":" + config_.api_token;
        std::string auth_encoded = base64_encode(auth);
        
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Basic " + auth_encoded;
        
        std::string url = config_.url;
        if (url.back() == '/') url.pop_back();
        url += "/rest/api/3/issue/" + external_id + "/transitions";
        
        CurlHelper curl;
        curl.post(url, json.str(), headers);
    } catch (...) {
        // Silently fail
    }
}

// Helper to convert severity to PagerDuty severity
static std::string severity_to_pagerduty(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::CRITICAL: return "critical";
        case IncidentSeverity::HIGH: return "error";
        case IncidentSeverity::MEDIUM: return "warning";
        case IncidentSeverity::LOW: return "info";
        default: return "warning";
    }
}

std::string PagerDutyIntegration::create_incident(const Incident& incident) {
    if (config_.integration_key.empty()) {
        return "PD-DISABLED";
    }
    
    try {
        // Build Events API v2 payload
        std::ostringstream json;
        json << "{"
             << "\"routing_key\": \"" << config_.integration_key << "\","
             << "\"event\": {"
             << "\"event_action\": \"trigger\","
             << "\"dedup_key\": \"" << incident.incident_id << "\","
             << "\"payload\": {"
             << "\"summary\": \"" << json_escape(incident.title) << "\","
             << "\"severity\": \"" << severity_to_pagerduty(incident.severity) << "\","
             << "\"source\": \"agentlog\","
             << "\"component\": \"payment-gateway\","
             << "\"custom_details\": {\"incident_id\": \"" << incident.incident_id << "\"}"  
             << "}}}";
        
        CurlHelper curl;
        std::string pd_url = "http://localhost:8081/v2/enqueue";
        auto response = curl.post(pd_url, json.str());
        
        if (response.success && response.status_code == 202) {
            // Parse dedup_key from response
            std::string body = response.body;
            size_t key_pos = body.find("\"dedup_key\":\"");
            if (key_pos != std::string::npos) {
                key_pos += 13;
                size_t end_pos = body.find("\"", key_pos);
                return body.substr(key_pos, end_pos - key_pos);
            }
            return "PD-" + incident.incident_id;
        } else {
            std::cerr << "PagerDuty API error: " << response.status_code << " - " << response.body << "\n";
            return "PD-ERROR";
        }
    } catch (const std::exception& e) {
        std::cerr << "PagerDuty integration exception: " << e.what() << "\n";
        return "PD-EXCEPTION";
    }
}

void PagerDutyIntegration::update_incident(const std::string& external_id, const Incident& incident) {
    // PagerDuty Events API v2 doesn't have update - would need Change Events API
    // For now, we'll send an annotate event (note)
    if (config_.integration_key.empty() || external_id.find("PD-") != 0) {
        return;
    }
    
    // Note: This is a simplified version - real implementation would use Change Events API
}

void PagerDutyIntegration::resolve_incident(const std::string& external_id, const std::string& resolution) {
    if (config_.integration_key.empty() || external_id.find("PD-") != 0) {
        return;
    }
    
    try {
        // Send resolve event
        std::ostringstream json;
        json << "{"
             << "\"routing_key\": \"" << config_.integration_key << "\","
             << "\"event_action\": \"resolve\","
             << "\"dedup_key\": \"" << external_id << "\""
             << "}";
        
        CurlHelper curl;
        std::string pd_url = "http://localhost:8081/v2/enqueue";
        curl.post(pd_url, json.str());
    } catch (...) {
        // Silently fail
    }
}

// Helper to convert severity to Slack color
static std::string severity_to_slack_color(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::CRITICAL: return "#FF0000";  // Red
        case IncidentSeverity::HIGH: return "#FF6600";      // Orange
        case IncidentSeverity::MEDIUM: return "#FFCC00";    // Yellow
        case IncidentSeverity::LOW: return "#36A64F";       // Green
        default: return "#CCCCCC";
    }
}

// Helper to convert severity to emoji
static std::string severity_to_emoji(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::CRITICAL: return ":fire:";
        case IncidentSeverity::HIGH: return ":warning:";
        case IncidentSeverity::MEDIUM: return ":large_orange_diamond:";
        case IncidentSeverity::LOW: return ":information_source:";
        default: return ":grey_question:";
    }
}

std::string SlackIntegration::create_incident(const Incident& incident) {
    if (config_.webhook_url.empty()) {
        return "SLACK-DISABLED";
    }
    
    try {
        // Build Slack webhook payload with rich formatting
        std::ostringstream json;
        json << "{"
             << "\"text\": \"" << severity_to_emoji(incident.severity) << " New Incident: " << json_escape(incident.title) << "\","
             << "\"attachments\": [{"
             << "\"color\": \"" << severity_to_slack_color(incident.severity) << "\","
             << "\"fields\": ["
             << "{\"title\": \"Incident ID\", \"value\": \"" << incident.incident_id << "\", \"short\": true},"
             << "{\"title\": \"Severity\", \"value\": \"" << incident_severity_to_string(incident.severity) << "\", \"short\": true},"
             << "{\"title\": \"Description\", \"value\": \"" << json_escape(incident.description) << "\", \"short\": false},"
             << "{\"title\": \"Events\", \"value\": \"" << incident.event_ids.size() << " related events\", \"short\": true}"
             << "],"
             << "\"footer\": \"AgentLog\","
             << "\"ts\": " << std::chrono::duration_cast<std::chrono::seconds>(
                    incident.created_at.time_since_epoch()).count()
             << "}]";
        
        if (!config_.channel.empty()) {
            json << ",\"channel\": \"" << json_escape(config_.channel) << "\"";
        }
        json << "}";
        
        CurlHelper curl;
        auto response = curl.post(config_.webhook_url, json.str());
        
        if (response.success && response.status_code == 200) {
            return "SLACK-" + incident.incident_id;
        } else {
            std::cerr << "Slack webhook error: " << response.status_code << " - " << response.body << "\n";
            return "SLACK-ERROR";
        }
    } catch (const std::exception& e) {
        std::cerr << "Slack integration exception: " << e.what() << "\n";
        return "SLACK-EXCEPTION";
    }
}

void SlackIntegration::update_incident(const std::string& external_id, const Incident& incident) {
    // Slack webhooks are one-way - cannot update messages
    // Would need Slack API with message timestamp for updates
    // For now, send a new message
    if (config_.webhook_url.empty() || external_id.find("SLACK-") != 0) {
        return;
    }
    
    try {
        std::ostringstream json;
        json << "{\"text\": \":arrows_counterclockwise: Incident Updated: " << incident.title << "\"}";
        
        CurlHelper curl;
        curl.post(config_.webhook_url, json.str());
    } catch (...) {
        // Silently fail
    }
}

void SlackIntegration::resolve_incident(const std::string& external_id, const std::string& resolution) {
    if (config_.webhook_url.empty() || external_id.find("SLACK-") != 0) {
        return;
    }
    
    try {
        std::ostringstream json;
        json << "{\"text\": \":white_check_mark: Incident Resolved: " << external_id 
             << "\\nResolution: " << resolution << "\"}";
        
        CurlHelper curl;
        curl.post(config_.webhook_url, json.str());
    } catch (...) {
        // Silently fail
    }
}

} // namespace agentlog
