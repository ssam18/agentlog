// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/pattern_engine.h"
#include <algorithm>
#include <sstream>

namespace agentlog {

//=============================================================================
// SequentialPattern Implementation
//=============================================================================

double SequentialPattern::match(const LogEvent& event, 
                               const std::deque<LogEvent>& context) {
    if (steps_.empty()) return 0.0;
    
    // Check if current event matches the last step
    const auto& last_step = steps_.back();
    if (!matches_step(event, last_step)) {
        return 0.0;
    }
    
    // If single-step pattern, we're done
    if (steps_.size() == 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        match_count_++;
        return 1.0;
    }
    
    // For multi-step patterns, look for previous steps in context
    size_t current_step = steps_.size() - 1;
    auto current_time = event.timestamp();
    
    // Walk backwards through context to find matching steps
    for (auto it = context.rbegin(); it != context.rend() && current_step > 0; ++it) {
        const auto& prev_step = steps_[current_step - 1];
        
        // Check time constraint
        auto time_diff = std::chrono::duration_cast<duration_t>(
            current_time - it->timestamp()
        );
        
        if (time_diff > prev_step.max_time_since_prev) {
            break;  // Too old, pattern broken
        }
        
        // Check if this event matches the previous step
        if (matches_step(*it, prev_step)) {
            current_step--;
            current_time = it->timestamp();
            
            if (current_step == 0) {
                // Found complete pattern!
                std::lock_guard<std::mutex> lock(mutex_);
                match_count_++;
                return 1.0;
            }
        }
    }
    
    // Calculate partial match score
    double progress = 1.0 - (static_cast<double>(current_step) / steps_.size());
    return progress * 0.5;  // Partial matches get lower scores
}

void SequentialPattern::train(const LogEvent& event) {
    // Sequential patterns don't need training
}

std::string SequentialPattern::description() const {
    std::ostringstream oss;
    oss << "Sequential pattern: ";
    for (size_t i = 0; i < steps_.size(); ++i) {
        if (i > 0) oss << " → ";
        oss << steps_[i].event_type;
    }
    oss << " (matched " << match_count_ << " times)";
    return oss.str();
}

bool SequentialPattern::matches_step(const LogEvent& event, const Step& step) const {
    // Check event type
    if (event.event_type() != step.event_type) {
        return false;
    }
    
    // Check required entities
    for (const auto& required : step.required_entities) {
        if (event.entities().find(required) == event.entities().end()) {
            return false;
        }
    }
    
    // Check entity matcher regex
    if (step.entity_matcher) {
        std::regex pattern(*step.entity_matcher);
        bool found_match = false;
        
        for (const auto& [key, value] : event.entities()) {
            if (std::regex_search(value, pattern)) {
                found_match = true;
                break;
            }
        }
        
        if (!found_match) return false;
    }
    
    return true;
}

//=============================================================================
// FrequencyPattern Implementation
//=============================================================================

double FrequencyPattern::match(const LogEvent& event, 
                              const std::deque<LogEvent>& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Only match events of our type
    if (event.event_type() != event_type_) {
        return 0.0;
    }
    
    auto now = event.timestamp();
    auto cutoff = now - window_;
    
    // Clean old entries
    while (!event_times_.empty() && event_times_.front() < cutoff) {
        event_times_.pop_front();
    }
    
    size_t count = event_times_.size();
    
    switch (type_) {
        case FrequencyType::BURST: {
            // Check if we're over threshold
            if (count >= threshold_) {
                // Calculate how much over threshold (score increases with excess)
                double excess = static_cast<double>(count - threshold_ + 1) / threshold_;
                return std::min(1.0, 0.7 + excess * 0.3);
            }
            return 0.0;
        }
        
        case FrequencyType::REPEATED: {
            // Check if any entity appears too frequently
            for (const auto& [entity_key, entity_value] : event.entities()) {
                auto& times = entity_times_[entity_value];
                
                // Clean old entries
                while (!times.empty() && times.front() < cutoff) {
                    times.pop_front();
                }
                
                if (times.size() >= threshold_) {
                    return 1.0;
                }
            }
            return 0.0;
        }
        
        case FrequencyType::ABSENCE: {
            // This type is checked differently - not implemented here
            return 0.0;
        }
    }
    
    return 0.0;
}

void FrequencyPattern::train(const LogEvent& event) {
    if (event.event_type() != event_type_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    event_times_.push_back(event.timestamp());
    
    // Track entities
    for (const auto& [key, value] : event.entities()) {
        entity_times_[value].push_back(event.timestamp());
    }
}

std::string FrequencyPattern::description() const {
    std::ostringstream oss;
    oss << "Frequency pattern: " << event_type_;
    
    switch (type_) {
        case FrequencyType::BURST:
            oss << " (burst detection, threshold=" << threshold_ << ")";
            break;
        case FrequencyType::REPEATED:
            oss << " (repeated entity, threshold=" << threshold_ << ")";
            break;
        case FrequencyType::ABSENCE:
            oss << " (absence detection)";
            break;
    }
    
    return oss.str();
}

//=============================================================================
// RegexPattern Implementation
//=============================================================================

double RegexPattern::match(const LogEvent& event, 
                          const std::deque<LogEvent>& context) {
    std::string value;
    
    if (field_ == "message") {
        value = event.message();
    } else if (field_ == "event_type") {
        value = event.event_type();
    } else {
        // Check entities
        auto it = event.entities().find(field_);
        if (it != event.entities().end()) {
            value = it->second;
        } else {
            return 0.0;
        }
    }
    
    return std::regex_search(value, regex_) ? 1.0 : 0.0;
}

std::string RegexPattern::description() const {
    return "Regex pattern: " + pattern_ + " in field '" + field_ + "'";
}

//=============================================================================
// PatternEngine Implementation
//=============================================================================

void PatternEngine::register_pattern(std::shared_ptr<PatternMatcher> pattern) {
    std::lock_guard<std::mutex> lock(mutex_);
    patterns_.push_back(std::move(pattern));
}

std::vector<PatternEngine::PatternMatch> PatternEngine::match_patterns(
    const LogEvent& event,
    const std::deque<LogEvent>& context) {
    
    std::vector<PatternMatch> matches;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pattern : patterns_) {
        double score = pattern->match(event, context);
        if (score > 0.5) {  // Only report significant matches
            matches.push_back({
                pattern,
                score,
                pattern->description()
            });
        }
    }
    
    // Sort by score (highest first)
    std::sort(matches.begin(), matches.end(),
        [](const auto& a, const auto& b) { return a.score > b.score; });
    
    return matches;
}

void PatternEngine::train_all(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pattern : patterns_) {
        pattern->train(event);
    }
}

std::vector<std::shared_ptr<PatternMatcher>> PatternEngine::patterns() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return patterns_;
}

void PatternEngine::register_builtin_patterns() {
    register_pattern(PatternFactory::cascading_failure());
    register_pattern(PatternFactory::auth_failure_pattern());
    register_pattern(PatternFactory::retry_storm());
    register_pattern(PatternFactory::exception_pattern());
}

//=============================================================================
// PatternFactory Implementation
//=============================================================================

std::shared_ptr<SequentialPattern> PatternFactory::cascading_failure() {
    std::vector<SequentialPattern::Step> steps;
    
    steps.emplace_back("database.slow", std::chrono::seconds(10));
    steps.emplace_back("api.timeout", std::chrono::seconds(5));
    steps.emplace_back("user.error", std::chrono::seconds(3));
    
    return std::make_shared<SequentialPattern>("cascading_failure", std::move(steps));
}

std::shared_ptr<FrequencyPattern> PatternFactory::auth_failure_pattern() {
    return std::make_shared<FrequencyPattern>(
        "auth_failure_burst",
        "auth.failed",
        FrequencyPattern::FrequencyType::REPEATED,
        5,  // 5 failures
        std::chrono::seconds(60)
    );
}

std::shared_ptr<FrequencyPattern> PatternFactory::retry_storm() {
    return std::make_shared<FrequencyPattern>(
        "retry_storm",
        "api.retry",
        FrequencyPattern::FrequencyType::BURST,
        10,  // 10 retries
        std::chrono::seconds(30)
    );
}

std::shared_ptr<SequentialPattern> PatternFactory::memory_leak_pattern() {
    std::vector<SequentialPattern::Step> steps;
    
    steps.emplace_back("memory.high", std::chrono::minutes(5));
    steps.emplace_back("gc.frequent", std::chrono::minutes(2));
    steps.emplace_back("oom.warning", std::chrono::minutes(1));
    
    return std::make_shared<SequentialPattern>("memory_leak", std::move(steps));
}

std::shared_ptr<RegexPattern> PatternFactory::exception_pattern() {
    return std::make_shared<RegexPattern>(
        "exception_detected",
        "Exception|Error|Traceback|at \\w+\\.\\w+\\(",
        "message"
    );
}

} // namespace agentlog
