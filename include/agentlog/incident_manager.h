// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include "event.h"
#include "correlation_engine.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace agentlog {

// Forward declarations
class IncidentIntegration;

/**
 * @brief Severity levels for incidents
 */
enum class IncidentSeverity {
    LOW,        // Minor issue, no immediate action needed
    MEDIUM,     // Notable issue, should be addressed
    HIGH,       // Serious issue, requires attention
    CRITICAL    // System-impacting, immediate action required
};

inline const char* incident_severity_to_string(IncidentSeverity s) {
    switch (s) {
        case IncidentSeverity::LOW: return "LOW";
        case IncidentSeverity::MEDIUM: return "MEDIUM";
        case IncidentSeverity::HIGH: return "HIGH";
        case IncidentSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Status of an incident
 */
enum class IncidentStatus {
    OPEN,       // Newly created
    INVESTIGATING, // Being looked at
    IDENTIFIED,  // Root cause found
    MONITORING,  // Issue fixed, monitoring for recurrence
    RESOLVED,    // Completely resolved
    CLOSED       // Closed without resolution
};

/**
 * @brief Represents an incident/ticket
 */
struct Incident {
    std::string incident_id;
    std::string title;
    std::string description;
    IncidentSeverity severity;
    IncidentStatus status;
    
    timestamp_t created_at;
    std::optional<timestamp_t> resolved_at;
    
    // Related events
    std::vector<uint64_t> event_ids;
    
    // Root cause analysis
    std::optional<std::string> root_cause;
    std::optional<uint64_t> root_cause_event_id;
    
    // Metrics
    double anomaly_score;
    size_t affected_services_count;
    size_t affected_users_count;
    
    // Context
    std::unordered_map<std::string, std::string> labels;
    std::vector<std::string> tags;
    
    // External system IDs (when synced)
    std::optional<std::string> jira_ticket_id;
    std::optional<std::string> pagerduty_incident_id;
    
    std::string to_json() const;
    std::string summary() const;
};

/**
 * @brief Callback for incident lifecycle events
 */
using IncidentCallback = std::function<void(const Incident&)>;

/**
 * @brief Manages incident creation and lifecycle
 */
/**
 * @brief Configuration for incident creation
 */
struct IncidentManagerConfig {
    // Thresholds
    double anomaly_threshold{0.75};           // Min anomaly score to create incident
    size_t pattern_match_threshold{1};        // Min pattern matches to create incident
    size_t correlated_events_threshold{3};    // Min correlated events for incident
    
    // Auto-resolution
    bool enable_auto_resolution{true};
    duration_t resolution_timeout{std::chrono::minutes(15)};
    
    // Deduplication
    bool enable_deduplication{true};
    duration_t deduplication_window{std::chrono::minutes(5)};
    
    // Severity mapping
    double critical_threshold{0.95};
    double high_threshold{0.85};
    double medium_threshold{0.75};
};

class IncidentManager {
public:
    using Config = IncidentManagerConfig;
    
    IncidentManager(Config config = Config())
        : config_(std::move(config))
        , next_incident_id_(1) {}
    
    /**
     * @brief Evaluate if event should create incident
     * @return Created incident, or nullopt if no incident created
     */
    std::optional<Incident> evaluate_event(
        const LogEvent& event,
        const std::vector<Correlation>& correlations = {},
        const std::vector<std::string>& matched_patterns = {}
    );
    
    /**
     * @brief Create incident manually
     */
    Incident create_incident(
        const std::string& title,
        const std::string& description,
        IncidentSeverity severity,
        const std::vector<uint64_t>& event_ids = {}
    );
    
    /**
     * @brief Update incident status
     */
    void update_status(const std::string& incident_id, IncidentStatus new_status);
    
    /**
     * @brief Resolve incident
     */
    void resolve_incident(const std::string& incident_id, const std::string& resolution);
    
    /**
     * @brief Get incident by ID
     */
    std::optional<Incident> get_incident(const std::string& incident_id) const;
    
    /**
     * @brief Get all open incidents
     */
    std::vector<Incident> get_open_incidents() const;
    
    /**
     * @brief Get all incidents
     */
    std::vector<Incident> get_all_incidents() const;
    
    /**
     * @brief Check for duplicate incidents (deduplication)
     */
    std::optional<std::string> find_duplicate(const Incident& incident) const;
    
    /**
     * @brief Auto-resolve old incidents
     */
    void auto_resolve_stale_incidents();
    
    /**
     * @brief Register external integration (Jira, PagerDuty, Slack, etc.)
     */
    void register_integration(std::shared_ptr<IncidentIntegration> integration);
    
    /**
     * @brief Register callback for incident creation
     */
    void on_incident_created(IncidentCallback callback);
    
    /**
     * @brief Register callback for incident resolution
     */
    void on_incident_resolved(IncidentCallback callback);
    
    /**
     * @brief Get statistics
     */
    struct Stats {
        size_t total_created{0};
        size_t currently_open{0};
        size_t resolved{0};
        size_t deduplicated{0};
    };
    
    Stats get_stats() const;
    
private:
    IncidentSeverity calculate_severity(
        double anomaly_score,
        size_t pattern_matches,
        size_t correlated_events
    ) const;
    
    std::string generate_incident_id();
    
    Config config_;
    std::atomic<uint64_t> next_incident_id_;
    
    std::unordered_map<std::string, Incident> incidents_;
    mutable std::mutex mutex_;
    
    std::vector<IncidentCallback> on_created_callbacks_;
    std::vector<IncidentCallback> on_resolved_callbacks_;
    
    // External integrations (Phase 3)
    std::vector<std::shared_ptr<IncidentIntegration>> integrations_;
    
    Stats stats_;
};

/**
 * @brief External integrations for incident management
 */
class IncidentIntegration {
public:
    virtual ~IncidentIntegration() = default;
    
    /**
     * @brief Create incident in external system
     * @return External incident ID
     */
    virtual std::string create_incident(const Incident& incident) = 0;
    
    /**
     * @brief Update incident in external system
     */
    virtual void update_incident(const std::string& external_id, const Incident& incident) = 0;
    
    /**
     * @brief Resolve incident in external system
     */
    virtual void resolve_incident(const std::string& external_id, const std::string& resolution) = 0;
    
    /**
     * @brief Get integration name
     */
    virtual std::string name() const = 0;
};

/**
 * @brief Jira integration (placeholder - would need actual REST API calls)
 */
class JiraIntegration : public IncidentIntegration {
public:
    struct Config {
        std::string url;
        std::string username;
        std::string api_token;
        std::string project_key;
    };
    
    JiraIntegration(Config config) : config_(std::move(config)) {}
    
    std::string create_incident(const Incident& incident) override;
    void update_incident(const std::string& external_id, const Incident& incident) override;
    void resolve_incident(const std::string& external_id, const std::string& resolution) override;
    std::string name() const override { return "Jira"; }
    
private:
    Config config_;
};

/**
 * @brief PagerDuty integration (placeholder - would need actual API calls)
 */
class PagerDutyIntegration : public IncidentIntegration {
public:
    struct Config {
        std::string integration_key;
        std::string api_token;
    };
    
    PagerDutyIntegration(Config config) : config_(std::move(config)) {}
    
    std::string create_incident(const Incident& incident) override;
    void update_incident(const std::string& external_id, const Incident& incident) override;
    void resolve_incident(const std::string& external_id, const std::string& resolution) override;
    std::string name() const override { return "PagerDuty"; }
    
private:
    Config config_;
};

/**
 * @brief Slack integration for notifications (placeholder)
 */
class SlackIntegration : public IncidentIntegration {
public:
    struct Config {
        std::string webhook_url;
        std::string channel;
    };
    
    SlackIntegration(Config config) : config_(std::move(config)) {}
    
    std::string create_incident(const Incident& incident) override;
    void update_incident(const std::string& external_id, const Incident& incident) override;
    void resolve_incident(const std::string& external_id, const std::string& resolution) override;
    std::string name() const override { return "Slack"; }
    
private:
    Config config_;
};

} // namespace agentlog
