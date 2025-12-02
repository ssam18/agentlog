// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include "event.h"
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>

namespace agentlog {

/**
 * @brief Represents a correlation between events
 */
struct Correlation {
    std::vector<uint64_t> event_ids;      // IDs of correlated events
    std::string correlation_type;          // Type of correlation
    double confidence;                     // Confidence score 0.0-1.0
    std::string reason;                    // Why these events are correlated
    timestamp_t first_event_time;
    timestamp_t last_event_time;
    
    // Additional context
    std::unordered_map<std::string, std::string> metadata;
};

/**
 * @brief Correlates events across services using various strategies
 */
class EventCorrelator {
public:
    EventCorrelator() = default;
    
    /**
     * @brief Add event and find correlations
     * @return Vector of correlations found
     */
    std::vector<Correlation> correlate(const LogEvent& event);
    
    /**
     * @brief Get all correlations involving a specific event
     */
    std::vector<Correlation> get_correlations_for_event(uint64_t event_id) const;
    
    /**
     * @brief Get all active correlations
     */
    std::vector<Correlation> get_active_correlations() const;
    
    /**
     * @brief Clear old correlations (cleanup)
     */
    void cleanup(duration_t max_age = std::chrono::hours(1));
    
private:
    struct EventRecord {
        LogEvent event;
        std::vector<size_t> correlation_indices;
    };
    
    // Find correlations by trace ID
    std::optional<Correlation> correlate_by_trace_id(const LogEvent& event);
    
    // Find correlations by entity matching
    std::optional<Correlation> correlate_by_entities(const LogEvent& event);
    
    // Find correlations by service relationships
    std::optional<Correlation> correlate_by_service(const LogEvent& event);
    
    // Find correlations by temporal proximity
    std::optional<Correlation> correlate_by_time(const LogEvent& event);
    
    std::unordered_map<uint64_t, EventRecord> events_;
    std::vector<Correlation> correlations_;
    
    // Index structures for fast lookup
    std::unordered_map<std::string, std::vector<uint64_t>> trace_id_index_;
    std::unordered_map<std::string, std::vector<uint64_t>> entity_index_;
    std::unordered_map<std::string, std::vector<uint64_t>> service_index_;
    
    mutable std::mutex mutex_;
};

/**
 * @brief Analyzes causality relationships between events
 * 
 * Detects patterns like:
 * - A causes B (e.g., database slow → API timeout)
 * - A prevents B (e.g., circuit breaker → no downstream calls)
 * - A enables B (e.g., auth success → protected resource access)
 */
class CausalityAnalyzer {
public:
    enum class CausalityType {
        CAUSES,      // A causes B
        PREVENTS,    // A prevents B
        ENABLES,     // A enables B
        PRECEDES     // A precedes B (temporal only)
    };
    
    struct CausalRelationship {
        std::string cause_event_type;
        std::string effect_event_type;
        CausalityType type;
        double strength;          // 0.0-1.0 confidence
        duration_t typical_delay;  // Typical time between cause and effect
        uint64_t observed_count;   // How many times observed
        
        std::string description() const;
    };
    
    CausalityAnalyzer() = default;
    
    /**
     * @brief Analyze event for causal relationships
     */
    std::vector<CausalRelationship> analyze(
        const LogEvent& event,
        const std::deque<LogEvent>& context
    );
    
    /**
     * @brief Learn causal relationships from event stream
     */
    void learn(const LogEvent& event, const std::deque<LogEvent>& context);
    
    /**
     * @brief Get all known causal relationships
     */
    std::vector<CausalRelationship> get_known_relationships() const;
    
    /**
     * @brief Register a known causal relationship
     */
    void register_relationship(const CausalRelationship& rel);
    
private:
    struct EventPair {
        std::string cause_type;
        std::string effect_type;
        
        bool operator==(const EventPair& other) const {
            return cause_type == other.cause_type && effect_type == other.effect_type;
        }
    };
    
    struct EventPairHash {
        size_t operator()(const EventPair& pair) const {
            return std::hash<std::string>()(pair.cause_type) ^
                   (std::hash<std::string>()(pair.effect_type) << 1);
        }
    };
    
    std::unordered_map<EventPair, CausalRelationship, EventPairHash> relationships_;
    mutable std::mutex mutex_;
};

/**
 * @brief Root cause analyzer - finds the root cause of issues
 */
class RootCauseAnalyzer {
public:
    struct RootCause {
        uint64_t root_event_id;
        std::string root_event_type;
        std::vector<uint64_t> affected_event_ids;
        double confidence;
        std::string explanation;
        
        // Evidence supporting this as root cause
        struct Evidence {
            std::string type;         // "temporal", "causal", "correlation"
            std::string description;
            double weight;
        };
        std::vector<Evidence> evidence;
    };
    
    RootCauseAnalyzer(
        std::shared_ptr<EventCorrelator> correlator,
        std::shared_ptr<CausalityAnalyzer> causality
    ) : correlator_(std::move(correlator))
      , causality_(std::move(causality)) {}
    
    /**
     * @brief Analyze correlation to find root cause
     */
    std::optional<RootCause> find_root_cause(const Correlation& correlation);
    
    /**
     * @brief Find root cause for a specific problematic event
     */
    std::optional<RootCause> find_root_cause_for_event(
        uint64_t event_id,
        const std::deque<LogEvent>& context
    );
    
private:
    std::shared_ptr<EventCorrelator> correlator_;
    std::shared_ptr<CausalityAnalyzer> causality_;
};

/**
 * @brief Correlation engine - main entry point
 */
class CorrelationEngine {
public:
    CorrelationEngine()
        : correlator_(std::make_shared<EventCorrelator>())
        , causality_(std::make_shared<CausalityAnalyzer>())
        , root_cause_(std::make_shared<RootCauseAnalyzer>(correlator_, causality_)) {}
    
    /**
     * @brief Process event and find correlations
     */
    void process(const LogEvent& event, const std::deque<LogEvent>& context);
    
    /**
     * @brief Get correlator
     */
    std::shared_ptr<EventCorrelator> correlator() { return correlator_; }
    
    /**
     * @brief Get causality analyzer
     */
    std::shared_ptr<CausalityAnalyzer> causality() { return causality_; }
    
    /**
     * @brief Get root cause analyzer
     */
    std::shared_ptr<RootCauseAnalyzer> root_cause() { return root_cause_; }
    
    /**
     * @brief Register built-in causal relationships
     */
    void register_builtin_relationships();
    
private:
    std::shared_ptr<EventCorrelator> correlator_;
    std::shared_ptr<CausalityAnalyzer> causality_;
    std::shared_ptr<RootCauseAnalyzer> root_cause_;
};

} // namespace agentlog
