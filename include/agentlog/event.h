// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include <atomic>
#include <optional>
#include <sstream>
#include <utility>

namespace agentlog {

/**
 * @brief Structured semantic log event with rich metadata
 * 
 * Core data structure representing a single log event with:
 * - Semantic event type (not just a message)
 * - Structured entities (users, services, endpoints, etc.)
 * - Numeric metrics for analysis
 * - Execution context
 * - AI-enhanced fields (anomaly scores, predictions)
 */
class LogEvent {
public:
    LogEvent() 
        : timestamp_(std::chrono::system_clock::now())
        , severity_(Severity::INFO)
        , anomaly_score_(0.0)
        , event_id_(generate_id())
    {}
    
    explicit LogEvent(std::string event_type)
        : event_type_(std::move(event_type))
        , timestamp_(std::chrono::system_clock::now())
        , severity_(Severity::INFO)
        , anomaly_score_(0.0)
        , event_id_(generate_id())
    {}
    
    // Fluent API for building events
    LogEvent& event_type(std::string type) {
        event_type_ = std::move(type);
        return *this;
    }
    
    LogEvent& severity(Severity s) {
        severity_ = s;
        return *this;
    }
    
    LogEvent& message(std::string msg) {
        message_ = std::move(msg);
        return *this;
    }
    
    // Add structured entity
    LogEvent& entity(const std::string& name, const std::string& value) {
        entities_[name] = value;
        return *this;
    }
    
    // Add numeric metric
    LogEvent& metric(const std::string& name, MetricValue value) {
        metrics_[name] = value;
        return *this;
    }
    
    // Add context information
    LogEvent& context(const std::string& key, const std::string& value) {
        context_[key] = value;
        return *this;
    }
    
    // Add tag
    LogEvent& tag(const std::string& t) {
        tags_.push_back(t);
        return *this;
    }
    
    // Add multiple tags
    LogEvent& tags(const std::vector<std::string>& t) {
        tags_.insert(tags_.end(), t.begin(), t.end());
        return *this;
    }
    
    // Capture stack trace
    LogEvent& capture_stack_trace(size_t max_frames = 32);
    
    // Capture local variables (implementation-specific)
    template<typename... Args>
    LogEvent& capture_variables(Args&&... args) {
        // TODO: Implement using debug info or custom serialization
        return *this;
    }
    
    // Service identification
    LogEvent& service_name(std::string name) {
        service_name_ = std::move(name);
        return *this;
    }
    
    LogEvent& service_instance(std::string instance) {
        service_instance_ = std::move(instance);
        return *this;
    }
    
    // Distributed tracing
    LogEvent& trace_id(std::string id) {
        trace_id_ = std::move(id);
        return *this;
    }
    
    LogEvent& span_id(std::string id) {
        span_id_ = std::move(id);
        return *this;
    }
    
    // AI-enhanced fields
    LogEvent& anomaly_score(double score) {
        anomaly_score_ = score;
        return *this;
    }
    
    LogEvent& predicted_label(const std::string& label) {
        predicted_labels_.push_back(label);
        return *this;
    }
    
    LogEvent& incident_id(std::string id) {
        incident_id_ = std::move(id);
        return *this;
    }
    
    // Getters
    const std::string& event_type() const { return event_type_; }
    timestamp_t timestamp() const { return timestamp_; }
    Severity severity() const { return severity_; }
    const std::string& message() const { return message_; }
    const ContextMap& entities() const { return entities_; }
    const MetricMap& metrics() const { return metrics_; }
    const ContextMap& context() const { return context_; }
    const std::vector<std::string>& tags() const { return tags_; }
    const StackTrace& stack_trace() const { return stack_trace_; }
    const std::string& service_name() const { return service_name_; }
    const std::string& trace_id() const { return trace_id_; }
    const std::string& span_id() const { return span_id_; }
    double anomaly_score() const { return anomaly_score_; }
    uint64_t event_id() const { return event_id_; }
    const std::optional<std::string>& incident_id() const { return incident_id_; }
    
    // Check if event has anomaly
    bool is_anomalous(double threshold = 0.7) const {
        return anomaly_score_ >= threshold;
    }
    
    // Serialization (for storage/transmission)
    std::string to_json() const;
    std::string to_string() const;
    
private:
    static uint64_t generate_id() {
        static std::atomic<uint64_t> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Core fields
    std::string event_type_;
    timestamp_t timestamp_;
    Severity severity_;
    std::string message_;
    
    // Structured data
    ContextMap entities_;      // Semantic entities
    MetricMap metrics_;        // Numeric metrics
    ContextMap context_;       // Additional context
    std::vector<std::string> tags_;
    StackTrace stack_trace_;
    
    // Service info
    std::string service_name_;
    std::string service_instance_;
    
    // Distributed tracing
    std::string trace_id_;
    std::string span_id_;
    
    // AI-enhanced
    double anomaly_score_;
    std::vector<std::string> predicted_labels_;
    std::optional<std::string> incident_id_;
    
    // Unique ID
    uint64_t event_id_;
};

/**
 * @brief Builder for creating log events with fluent API
 */
class EventBuilder {
public:
    explicit EventBuilder(std::string event_type)
        : event_(std::move(event_type)) {}
    
    EventBuilder& entity(const std::string& name, const std::string& value) {
        event_.entity(name, value);
        return *this;
    }
    
    EventBuilder& metric(const std::string& name, MetricValue value) {
        event_.metric(name, value);
        return *this;
    }
    
    EventBuilder& context(const std::string& key, const std::string& value) {
        event_.context(key, value);
        return *this;
    }
    
    EventBuilder& severity(Severity s) {
        event_.severity(s);
        return *this;
    }
    
    EventBuilder& message(std::string msg) {
        event_.message(std::move(msg));
        return *this;
    }
    
    EventBuilder& capture_stack_trace() {
        event_.capture_stack_trace();
        return *this;
    }
    
    // Emit the event (sends to processing pipeline)
    void emit();
    
    // Get the built event without emitting
    LogEvent build() { return std::move(event_); }
    
private:
    LogEvent event_;
};

} // namespace agentlog
