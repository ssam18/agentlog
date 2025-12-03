// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include "event.h"
#include <functional>
#include <mutex>
#include <thread>
#include <deque>
#include <fstream>

namespace agentlog {

/**
 * @brief Configuration for AgentLog
 */
struct Config {
    std::string service_name;
    std::string environment{"production"};
    std::string service_instance;
    
    // Sampling
    double sampling_rate{1.0};  // 1.0 = 100%, 0.1 = 10%
    bool sample_anomalies_always{true};  // Always keep anomalous events
    
    // Performance
    size_t async_queue_size{8192};
    size_t worker_threads{2};
    
    // AI features
    bool enable_anomaly_detection{true};
    bool enable_pattern_matching{true};
    bool enable_correlation{true};
    bool enable_prediction{false};  // Not implemented yet
    bool enable_auto_incidents{false};
    
    // Incident thresholds
    double incident_anomaly_threshold{0.8};
    size_t incident_pattern_threshold{1};
    size_t incident_correlation_threshold{3};
    
    // Storage
    std::string storage_path{"./agentlog_data"};
    size_t max_storage_mb{1024};  // 1GB default
    
    // File logging
    std::string log_file_path;     // If set, logs will be written to this file
    bool log_to_console{true};     // Log to stdout/stderr
    
    // Phase 3: External Integrations
    // Jira Cloud REST API configuration
    struct {
        std::string url;           // e.g., "https://your-domain.atlassian.net"
        std::string username;      // Jira username/email
        std::string api_token;     // API token from account settings
        std::string project_key;   // Project key (e.g., "PROJ")
        bool enabled{false};       // Enable Jira integration
    } jira;
    
    // PagerDuty Events API v2 configuration
    struct {
        std::string integration_key;  // Integration key from PagerDuty service
        std::string api_token;        // API token (optional, for additional features)
        bool enabled{false};          // Enable PagerDuty integration
    } pagerduty;
    
    // Slack Incoming Webhooks configuration
    struct {
        std::string webhook_url;   // Full webhook URL from Slack
        std::string channel;       // Optional: override default channel
        bool enabled{false};       // Enable Slack integration
    } slack;
};

/**
 * @brief Callback for processing events
 */
using EventCallback = std::function<void(const LogEvent&)>;

/**
 * @brief Main logger class - thread-safe singleton
 */
class Logger {
public:
    static Logger& instance();
    
    // Initialize with configuration
    void init(const Config& config);
    void shutdown();
    
    // Create event builders
    EventBuilder event(const std::string& event_type);
    EventBuilder observe(const std::string& metric_name);
    
    // Direct logging methods (for compatibility)
    void trace(const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void critical(const std::string& msg);
    
    // Emit event (called by EventBuilder)
    void emit(const LogEvent& event);
    
    // Register callbacks
    void on_event(EventCallback callback);
    void on_anomaly(EventCallback callback);
    
    // Configuration access
    const Config& config() const { return config_; }
    
    // Stats
    struct Stats {
        uint64_t events_total{0};
        uint64_t events_dropped{0};
        uint64_t anomalies_detected{0};
        uint64_t patterns_matched{0};
        uint64_t correlations_found{0};
        uint64_t incidents_created{0};
    };
    
    Stats get_stats() const;
    
    // Access to AI components (for advanced usage)
    PatternEnginePtr pattern_engine() const { return pattern_engine_; }
    CorrelationEnginePtr correlation_engine() const { return correlation_engine_; }
    IncidentManagerPtr incident_manager() const { return incident_manager_; }
    
private:
    Logger() = default;
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void process_event(const LogEvent& event);
    void async_worker();
    bool should_sample(const LogEvent& event) const;
    
    Config config_;
    bool initialized_{false};
    bool shutdown_requested_{false};
    
    std::mutex mutex_;
    std::mutex file_mutex_;  // Separate mutex for file I/O
    std::ofstream log_file_;   // File output stream
    std::vector<EventCallback> event_callbacks_;
    std::vector<EventCallback> anomaly_callbacks_;
    
    // Stats
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // Worker thread for async processing
    std::vector<std::thread> workers_;
    
    // Components (initialized in init())
    AnomalyDetectorPtr anomaly_detector_;
    PatternEnginePtr pattern_engine_;
    CorrelationEnginePtr correlation_engine_;
    IncidentManagerPtr incident_manager_;
    
    // Event history for pattern/correlation analysis
    std::deque<LogEvent> event_history_;
    size_t max_history_size_{1000};
};

/**
 * @brief Global convenience functions
 */
namespace global {

inline void init(const Config& config) {
    Logger::instance().init(config);
}

inline void shutdown() {
    Logger::instance().shutdown();
}

inline EventBuilder event(const std::string& event_type) {
    return Logger::instance().event(event_type);
}

inline EventBuilder observe(const std::string& metric_name) {
    return Logger::instance().observe(metric_name);
}

inline void trace(const std::string& msg) {
    Logger::instance().trace(msg);
}

inline void debug(const std::string& msg) {
    Logger::instance().debug(msg);
}

inline void info(const std::string& msg) {
    Logger::instance().info(msg);
}

inline void warn(const std::string& msg) {
    Logger::instance().warn(msg);
}

inline void error(const std::string& msg) {
    Logger::instance().error(msg);
}

inline void critical(const std::string& msg) {
    Logger::instance().critical(msg);
}

} // namespace global

} // namespace agentlog

// Convenience macros
#define AGENTLOG_EVENT(type) agentlog::global::event(type)
#define AGENTLOG_OBSERVE(metric) agentlog::global::observe(metric)
#define AGENTLOG_TRACE(msg) agentlog::global::trace(msg)
#define AGENTLOG_DEBUG(msg) agentlog::global::debug(msg)
#define AGENTLOG_INFO(msg) agentlog::global::info(msg)
#define AGENTLOG_WARN(msg) agentlog::global::warn(msg)
#define AGENTLOG_ERROR(msg) agentlog::global::error(msg)
#define AGENTLOG_CRITICAL(msg) agentlog::global::critical(msg)
