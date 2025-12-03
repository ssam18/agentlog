// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/logger.h"
#include "agentlog/anomaly_detector.h"
#include "agentlog/pattern_engine.h"
#include "agentlog/correlation_engine.h"
#include "agentlog/incident_manager.h"
#include <iostream>
#include <fstream>
#include <random>
#include <condition_variable>
#include <queue>

namespace agentlog {

namespace {
    // Thread-safe event queue
    class EventQueue {
    public:
        explicit EventQueue(size_t max_size) : max_size_(max_size), shutdown_(false) {}
        
        bool push(const LogEvent& event) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (queue_.size() >= max_size_) {
                return false;  // Queue full
            }
            queue_.push(event);
            cv_.notify_one();
            return true;
        }
        
        bool pop(LogEvent& event) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
            
            if (shutdown_ && queue_.empty()) {
                return false;
            }
            
            if (!queue_.empty()) {
                event = std::move(queue_.front());
                queue_.pop();
                return true;
            }
            
            return false;
        }
        
        void shutdown() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shutdown_ = true;
            }
            cv_.notify_all();
        }
        
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }
        
    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<LogEvent> queue_;
        size_t max_size_;
        bool shutdown_;
    };
}

// Global event queue
static std::unique_ptr<EventQueue> g_event_queue;

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::init(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        std::cerr << "AgentLog: Already initialized\n";
        return;
    }
    
    config_ = config;
    
    // Open log file if configured
    if (!config.log_file_path.empty()) {
        log_file_.open(config.log_file_path, std::ios::out | std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << config.log_file_path << std::endl;
        } else {
            std::cout << "Logging to file: " << config.log_file_path << std::endl;
        }
    }
    
    // Create event queue
    g_event_queue = std::make_unique<EventQueue>(config.async_queue_size);
    
    // Initialize AI components
    if (config.enable_anomaly_detection) {
        anomaly_detector_ = DetectorFactory::create_default();
    }
    
    if (config.enable_pattern_matching) {
        pattern_engine_ = std::make_shared<PatternEngine>();
        pattern_engine_->register_builtin_patterns();
    }
    
    if (config.enable_correlation) {
        correlation_engine_ = std::make_shared<CorrelationEngine>();
        correlation_engine_->register_builtin_relationships();
    }
    
    if (config.enable_auto_incidents) {
        IncidentManager::Config incident_config;
        incident_config.anomaly_threshold = config.incident_anomaly_threshold;
        incident_config.pattern_match_threshold = config.incident_pattern_threshold;
        incident_config.correlated_events_threshold = config.incident_correlation_threshold;
        
        incident_manager_ = std::make_shared<IncidentManager>(incident_config);
        
        // Configure external integrations
        if (config.jira.enabled && !config.jira.url.empty()) {
            JiraIntegration::Config jira_config;
            jira_config.url = config.jira.url;
            jira_config.username = config.jira.username;
            jira_config.api_token = config.jira.api_token;
            jira_config.project_key = config.jira.project_key;
            incident_manager_->register_integration(std::make_shared<JiraIntegration>(jira_config));
            std::cout << "Jira integration enabled: " << config.jira.url << std::endl;
        }
        
        if (config.pagerduty.enabled && !config.pagerduty.integration_key.empty()) {
            PagerDutyIntegration::Config pd_config;
            pd_config.integration_key = config.pagerduty.integration_key;
            pd_config.api_token = config.pagerduty.api_token;
            incident_manager_->register_integration(std::make_shared<PagerDutyIntegration>(pd_config));
            std::cout << "PagerDuty integration enabled" << std::endl;
        }
        
        if (config.slack.enabled && !config.slack.webhook_url.empty()) {
            SlackIntegration::Config slack_config;
            slack_config.webhook_url = config.slack.webhook_url;
            slack_config.channel = config.slack.channel;
            incident_manager_->register_integration(std::make_shared<SlackIntegration>(slack_config));
            std::cout << "Slack integration enabled: " << (!config.slack.channel.empty() ? config.slack.channel : "default") << std::endl;
        }
        
        // Register incident callbacks
        incident_manager_->on_incident_created([](const Incident& inc) {
            std::cout << "\n🎫 INCIDENT CREATED: " << inc.summary() << std::endl;
        });
    }
    
    // Start worker threads
    shutdown_requested_ = false;
    for (size_t i = 0; i < config.worker_threads; ++i) {
        workers_.emplace_back(&Logger::async_worker, this);
    }
    
    initialized_ = true;
    
    std::cout << "AgentLog v0.1.0 initialized for service: " 
              << config.service_name << std::endl;
    std::cout << "AI Features: "
              << (config.enable_anomaly_detection ? "Anomaly " : "")
              << (config.enable_pattern_matching ? "Pattern " : "")
              << (config.enable_correlation ? "Correlation " : "")
              << (config.enable_auto_incidents ? "Incidents" : "")
              << std::endl;
}

void Logger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return;
        shutdown_requested_ = true;
    }
    
    // Signal queue shutdown
    if (g_event_queue) {
        g_event_queue->shutdown();
    }
    
    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    g_event_queue.reset();
    
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
    
    std::cout << "AgentLog: Shutdown complete. Stats: "
              << stats_.events_total << " events, "
              << stats_.anomalies_detected << " anomalies, "
              << stats_.events_dropped << " dropped" << std::endl;
}

Logger::~Logger() {
    if (initialized_) {
        shutdown();
    }
}

EventBuilder Logger::event(const std::string& event_type) {
    EventBuilder builder(event_type);
    return builder;
}

EventBuilder Logger::observe(const std::string& metric_name) {
    EventBuilder builder("metric.observed");
    builder.context("metric_name", metric_name);
    return builder;
}

void Logger::trace(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::TRACE).message(msg);
    emit(event);
}

void Logger::debug(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::DEBUG).message(msg);
    emit(event);
}

void Logger::info(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::INFO).message(msg);
    emit(event);
}

void Logger::warn(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::WARNING).message(msg);
    emit(event);
}

void Logger::error(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::ERROR).message(msg);
    emit(event);
}

void Logger::critical(const std::string& msg) {
    LogEvent event("log.message");
    event.severity(Severity::CRITICAL).message(msg).capture_stack_trace();
    emit(event);
}

void Logger::emit(const LogEvent& event) {
    if (!initialized_) {
        // Fallback: print to stderr if not initialized
        std::cerr << event.to_string() << std::endl;
        return;
    }
    
    // Apply sampling
    if (!should_sample(event)) {
        return;
    }
    
    // Update stats
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.events_total++;
    }
    
    // Push to queue for async processing
    if (g_event_queue && !g_event_queue->push(event)) {
        // Queue full - drop event
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.events_dropped++;
    }
}

void Logger::async_worker() {
    while (!shutdown_requested_) {
        LogEvent event;
        if (g_event_queue && g_event_queue->pop(event)) {
            process_event(event);
        }
    }
    
    // Drain remaining events
    LogEvent event;
    while (g_event_queue && g_event_queue->pop(event)) {
        process_event(event);
    }
}

void Logger::process_event(const LogEvent& event) {
    LogEvent processed_event = event;
    
    // Apply anomaly detection
    if (anomaly_detector_ && !processed_event.metrics().empty()) {
        double score = anomaly_detector_->score(processed_event);
        processed_event.anomaly_score(score);
        
        // Train detector
        anomaly_detector_->train(processed_event);
        
        if (score >= 0.7) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.anomalies_detected++;
            
            // Trigger anomaly callbacks
            std::lock_guard<std::mutex> cb_lock(mutex_);
            for (const auto& callback : anomaly_callbacks_) {
                callback(processed_event);
            }
        }
    }
    
    // Pattern matching
    std::vector<std::string> matched_patterns;
    if (pattern_engine_) {
        auto matches = pattern_engine_->match_patterns(processed_event, event_history_);
        if (!matches.empty()) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.patterns_matched += matches.size();
            
            for (const auto& match : matches) {
                matched_patterns.push_back(match.pattern->name());
            }
        }
        pattern_engine_->train_all(processed_event);
    }
    
    // Correlation analysis
    std::vector<Correlation> correlations;
    if (correlation_engine_) {
        correlation_engine_->process(processed_event, event_history_);
        correlations = correlation_engine_->correlator()->correlate(processed_event);
        
        if (!correlations.empty()) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.correlations_found += correlations.size();
        }
    }
    
    // Incident management
    if (incident_manager_) {
        auto incident = incident_manager_->evaluate_event(
            processed_event, 
            correlations,
            matched_patterns
        );
        
        if (incident) {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.incidents_created++;
        }
    }
    
    // Add to event history
    {
        std::lock_guard<std::mutex> lock(mutex_);
        event_history_.push_back(processed_event);
        if (event_history_.size() > max_history_size_) {
            event_history_.pop_front();
        }
    }
    
    // Trigger event callbacks
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : event_callbacks_) {
            callback(processed_event);
        }
    }
    
    // Write to file if configured
    if (log_file_.is_open()) {
        std::lock_guard<std::mutex> file_lock(file_mutex_);
        if (!matched_patterns.empty()) {
            log_file_ << "[PATTERN:" << matched_patterns.front() << "] ";
        }
        log_file_ << processed_event.to_string() << std::endl;
    }
    
    // Print to console if enabled
    if (config_.log_to_console) {
        if (!matched_patterns.empty()) {
            std::cout << "🔍 PATTERN: " << matched_patterns.front() << " - ";
        }
        if (processed_event.is_anomalous()) {
            std::cout << "🔴 " << processed_event.to_string() << std::endl;
        } else if (processed_event.severity() >= Severity::WARNING) {
            std::cout << "🟡 " << processed_event.to_string() << std::endl;
        }
    }
}

bool Logger::should_sample(const LogEvent& event) const {
    // Always keep anomalies and high-severity events
    if (config_.sample_anomalies_always) {
        if (event.is_anomalous() || event.severity() >= Severity::ERROR) {
            return true;
        }
    }
    
    // Apply sampling rate
    if (config_.sampling_rate >= 1.0) {
        return true;
    }
    
    // Random sampling
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    return dis(gen) < config_.sampling_rate;
}

void Logger::on_event(EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callbacks_.push_back(std::move(callback));
}

void Logger::on_anomaly(EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    anomaly_callbacks_.push_back(std::move(callback));
}

Logger::Stats Logger::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

} // namespace agentlog
