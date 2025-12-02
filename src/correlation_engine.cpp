// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/correlation_engine.h"
#include <algorithm>
#include <sstream>

namespace agentlog {

//=============================================================================
// EventCorrelator Implementation
//=============================================================================

std::vector<Correlation> EventCorrelator::correlate(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Correlation> found_correlations;
    
    // Try different correlation strategies
    if (auto corr = correlate_by_trace_id(event)) {
        found_correlations.push_back(*corr);
    }
    
    if (auto corr = correlate_by_entities(event)) {
        found_correlations.push_back(*corr);
    }
    
    if (auto corr = correlate_by_service(event)) {
        found_correlations.push_back(*corr);
    }
    
    if (auto corr = correlate_by_time(event)) {
        found_correlations.push_back(*corr);
    }
    
    // Store event
    EventRecord record;
    record.event = event;
    events_[event.event_id()] = record;
    
    // Update indices
    if (!event.trace_id().empty()) {
        trace_id_index_[event.trace_id()].push_back(event.event_id());
    }
    
    for (const auto& [key, value] : event.entities()) {
        entity_index_[value].push_back(event.event_id());
    }
    
    if (!event.service_name().empty()) {
        service_index_[event.service_name()].push_back(event.event_id());
    }
    
    // Store correlations
    for (const auto& corr : found_correlations) {
        correlations_.push_back(corr);
    }
    
    return found_correlations;
}

std::optional<Correlation> EventCorrelator::correlate_by_trace_id(const LogEvent& event) {
    if (event.trace_id().empty()) {
        return std::nullopt;
    }
    
    auto it = trace_id_index_.find(event.trace_id());
    if (it == trace_id_index_.end() || it->second.empty()) {
        return std::nullopt;
    }
    
    Correlation corr;
    corr.correlation_type = "trace_id";
    corr.confidence = 1.0;
    corr.reason = "Events share trace ID: " + event.trace_id();
    corr.first_event_time = event.timestamp();
    corr.last_event_time = event.timestamp();
    
    corr.event_ids = it->second;
    corr.event_ids.push_back(event.event_id());
    
    corr.metadata["trace_id"] = event.trace_id();
    
    return corr;
}

std::optional<Correlation> EventCorrelator::correlate_by_entities(const LogEvent& event) {
    std::unordered_set<uint64_t> related_events;
    
    for (const auto& [key, value] : event.entities()) {
        auto it = entity_index_.find(value);
        if (it != entity_index_.end()) {
            for (uint64_t id : it->second) {
                if (id != event.event_id()) {
                    related_events.insert(id);
                }
            }
        }
    }
    
    if (related_events.empty()) {
        return std::nullopt;
    }
    
    Correlation corr;
    corr.correlation_type = "entity";
    corr.confidence = 0.8;
    corr.reason = "Events share common entities";
    corr.first_event_time = event.timestamp();
    corr.last_event_time = event.timestamp();
    
    corr.event_ids.assign(related_events.begin(), related_events.end());
    corr.event_ids.push_back(event.event_id());
    
    return corr;
}

std::optional<Correlation> EventCorrelator::correlate_by_service(const LogEvent& event) {
    if (event.service_name().empty()) {
        return std::nullopt;
    }
    
    auto it = service_index_.find(event.service_name());
    if (it == service_index_.end() || it->second.empty()) {
        return std::nullopt;
    }
    
    // Only correlate recent events from same service (last minute)
    std::vector<uint64_t> recent_events;
    auto cutoff = event.timestamp() - std::chrono::minutes(1);
    
    for (uint64_t id : it->second) {
        auto event_it = events_.find(id);
        if (event_it != events_.end() && 
            event_it->second.event.timestamp() >= cutoff) {
            recent_events.push_back(id);
        }
    }
    
    if (recent_events.empty()) {
        return std::nullopt;
    }
    
    Correlation corr;
    corr.correlation_type = "service";
    corr.confidence = 0.6;
    corr.reason = "Events from same service: " + event.service_name();
    corr.first_event_time = event.timestamp();
    corr.last_event_time = event.timestamp();
    corr.event_ids = recent_events;
    corr.event_ids.push_back(event.event_id());
    corr.metadata["service"] = event.service_name();
    
    return corr;
}

std::optional<Correlation> EventCorrelator::correlate_by_time(const LogEvent& event) {
    // Find events within 5 seconds
    std::vector<uint64_t> nearby_events;
    auto window = std::chrono::seconds(5);
    
    for (const auto& [id, record] : events_) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::abs(event.timestamp() - record.event.timestamp())
        );
        
        if (diff <= window && id != event.event_id()) {
            nearby_events.push_back(id);
        }
    }
    
    if (nearby_events.size() < 2) {
        return std::nullopt;
    }
    
    Correlation corr;
    corr.correlation_type = "temporal";
    corr.confidence = 0.4;
    corr.reason = "Events occurred within 5 seconds";
    corr.first_event_time = event.timestamp();
    corr.last_event_time = event.timestamp();
    corr.event_ids = nearby_events;
    corr.event_ids.push_back(event.event_id());
    
    return corr;
}

std::vector<Correlation> EventCorrelator::get_correlations_for_event(uint64_t event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Correlation> result;
    for (const auto& corr : correlations_) {
        if (std::find(corr.event_ids.begin(), corr.event_ids.end(), event_id) 
            != corr.event_ids.end()) {
            result.push_back(corr);
        }
    }
    
    return result;
}

std::vector<Correlation> EventCorrelator::get_active_correlations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return correlations_;
}

void EventCorrelator::cleanup(duration_t max_age) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - max_age;
    
    // Remove old events
    for (auto it = events_.begin(); it != events_.end();) {
        if (it->second.event.timestamp() < cutoff) {
            it = events_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove old correlations
    correlations_.erase(
        std::remove_if(correlations_.begin(), correlations_.end(),
            [cutoff](const Correlation& corr) {
                return corr.last_event_time < cutoff;
            }),
        correlations_.end()
    );
    
    // Rebuild indices
    trace_id_index_.clear();
    entity_index_.clear();
    service_index_.clear();
    
    for (const auto& [id, record] : events_) {
        const auto& event = record.event;
        
        if (!event.trace_id().empty()) {
            trace_id_index_[event.trace_id()].push_back(id);
        }
        
        for (const auto& [key, value] : event.entities()) {
            entity_index_[value].push_back(id);
        }
        
        if (!event.service_name().empty()) {
            service_index_[event.service_name()].push_back(id);
        }
    }
}

//=============================================================================
// CausalityAnalyzer Implementation
//=============================================================================

std::string CausalityAnalyzer::CausalRelationship::description() const {
    std::ostringstream oss;
    oss << cause_event_type;
    
    switch (type) {
        case CausalityType::CAUSES:
            oss << " causes ";
            break;
        case CausalityType::PREVENTS:
            oss << " prevents ";
            break;
        case CausalityType::ENABLES:
            oss << " enables ";
            break;
        case CausalityType::PRECEDES:
            oss << " precedes ";
            break;
    }
    
    oss << effect_event_type;
    oss << " (strength=" << strength << ", observed=" << observed_count << "x)";
    
    return oss.str();
}

std::vector<CausalityAnalyzer::CausalRelationship> CausalityAnalyzer::analyze(
    const LogEvent& event,
    const std::deque<LogEvent>& context) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CausalRelationship> found;
    
    // Look for known causal relationships
    for (const auto& prev_event : context) {
        EventPair pair{prev_event.event_type(), event.event_type()};
        
        auto it = relationships_.find(pair);
        if (it != relationships_.end()) {
            found.push_back(it->second);
        }
    }
    
    return found;
}

void CausalityAnalyzer::learn(const LogEvent& event, const std::deque<LogEvent>& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Look at recent events (last 60 seconds)
    auto cutoff = event.timestamp() - std::chrono::seconds(60);
    
    for (const auto& prev_event : context) {
        if (prev_event.timestamp() < cutoff) {
            continue;
        }
        
        EventPair pair{prev_event.event_type(), event.event_type()};
        
        auto& rel = relationships_[pair];
        if (rel.observed_count == 0) {
            // New relationship
            rel.cause_event_type = prev_event.event_type();
            rel.effect_event_type = event.event_type();
            rel.type = CausalityType::PRECEDES;
            rel.strength = 0.1;
            rel.typical_delay = event.timestamp() - prev_event.timestamp();
        }
        
        rel.observed_count++;
        
        // Update typical delay (running average)
        auto delay = event.timestamp() - prev_event.timestamp();
        rel.typical_delay = (rel.typical_delay * (rel.observed_count - 1) + delay) / rel.observed_count;
        
        // Increase strength with more observations
        rel.strength = std::min(1.0, rel.strength + 0.05);
    }
}

std::vector<CausalityAnalyzer::CausalRelationship> CausalityAnalyzer::get_known_relationships() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CausalRelationship> result;
    for (const auto& [pair, rel] : relationships_) {
        result.push_back(rel);
    }
    
    return result;
}

void CausalityAnalyzer::register_relationship(const CausalRelationship& rel) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    EventPair pair{rel.cause_event_type, rel.effect_event_type};
    relationships_[pair] = rel;
}

//=============================================================================
// RootCauseAnalyzer Implementation
//=============================================================================

std::optional<RootCauseAnalyzer::RootCause> RootCauseAnalyzer::find_root_cause(
    const Correlation& correlation) {
    
    if (correlation.event_ids.empty()) {
        return std::nullopt;
    }
    
    // Simple heuristic: earliest event in correlation is likely root cause
    // In production, this would use ML and graph analysis
    
    RootCause root;
    root.root_event_id = correlation.event_ids.front();
    root.affected_event_ids = correlation.event_ids;
    root.confidence = correlation.confidence * 0.7;
    root.explanation = "Earliest event in correlation chain";
    
    // Add evidence
    root.evidence.push_back({
        "temporal",
        "First event in time sequence",
        0.6
    });
    
    root.evidence.push_back({
        "correlation",
        correlation.reason,
        correlation.confidence
    });
    
    return root;
}

std::optional<RootCauseAnalyzer::RootCause> RootCauseAnalyzer::find_root_cause_for_event(
    uint64_t event_id,
    const std::deque<LogEvent>& context) {
    
    // Get correlations for this event
    auto correlations = correlator_->get_correlations_for_event(event_id);
    
    if (correlations.empty()) {
        return std::nullopt;
    }
    
    // Use the strongest correlation
    auto best_corr = *std::max_element(correlations.begin(), correlations.end(),
        [](const Correlation& a, const Correlation& b) {
            return a.confidence < b.confidence;
        });
    
    return find_root_cause(best_corr);
}

//=============================================================================
// CorrelationEngine Implementation
//=============================================================================

void CorrelationEngine::process(const LogEvent& event, const std::deque<LogEvent>& context) {
    // Find correlations
    correlator_->correlate(event);
    
    // Learn causal relationships
    causality_->learn(event, context);
    
    // Analyze causality
    auto causal_rels = causality_->analyze(event, context);
    
    // Could trigger callbacks here for significant correlations/causality
}

void CorrelationEngine::register_builtin_relationships() {
    // Database → API causality
    CausalityAnalyzer::CausalRelationship db_api;
    db_api.cause_event_type = "database.slow";
    db_api.effect_event_type = "api.timeout";
    db_api.type = CausalityAnalyzer::CausalityType::CAUSES;
    db_api.strength = 0.9;
    db_api.typical_delay = std::chrono::milliseconds(500);
    db_api.observed_count = 100;
    causality_->register_relationship(db_api);
    
    // API → User error causality
    CausalityAnalyzer::CausalRelationship api_user;
    api_user.cause_event_type = "api.timeout";
    api_user.effect_event_type = "user.error";
    api_user.type = CausalityAnalyzer::CausalityType::CAUSES;
    api_user.strength = 0.8;
    api_user.typical_delay = std::chrono::milliseconds(100);
    api_user.observed_count = 100;
    causality_->register_relationship(api_user);
    
    // Circuit breaker → No calls
    CausalityAnalyzer::CausalRelationship circuit_nocall;
    circuit_nocall.cause_event_type = "circuit_breaker.open";
    circuit_nocall.effect_event_type = "api.call";
    circuit_nocall.type = CausalityAnalyzer::CausalityType::PREVENTS;
    circuit_nocall.strength = 1.0;
    circuit_nocall.typical_delay = std::chrono::milliseconds(0);
    circuit_nocall.observed_count = 100;
    causality_->register_relationship(circuit_nocall);
}

} // namespace agentlog
