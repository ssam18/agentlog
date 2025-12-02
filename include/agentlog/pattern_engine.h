// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include "event.h"
#include <deque>
#include <regex>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace agentlog {

/**
 * @brief Base class for pattern matching
 */
class PatternMatcher {
public:
    virtual ~PatternMatcher() = default;
    
    /**
     * @brief Check if event matches this pattern
     * @param event The event to check
     * @param context Historical context (recent events)
     * @return Score between 0.0 (no match) and 1.0 (perfect match)
     */
    virtual double match(const LogEvent& event, 
                        const std::deque<LogEvent>& context) = 0;
    
    /**
     * @brief Learn from observed events
     */
    virtual void train(const LogEvent& event) = 0;
    
    /**
     * @brief Get pattern name/description
     */
    virtual std::string name() const = 0;
    
    /**
     * @brief Get human-readable description of what this pattern detects
     */
    virtual std::string description() const = 0;
};

/**
 * @brief Detects sequential patterns (e.g., A → B → C within time window)
 * 
 * Example: "database.slow" followed by "api.timeout" followed by "user.error"
 * indicates a cascading failure pattern.
 */
class SequentialPattern : public PatternMatcher {
public:
    struct Step {
        std::string event_type;
        std::vector<std::string> required_entities;  // Must have these entities
        std::optional<std::string> entity_matcher;   // Regex for entity values
        duration_t max_time_since_prev;              // Max time from previous step
        
        Step(std::string type, duration_t max_time = std::chrono::seconds(60))
            : event_type(std::move(type))
            , max_time_since_prev(max_time) {}
    };
    
    SequentialPattern(std::string name, std::vector<Step> steps)
        : name_(std::move(name))
        , steps_(std::move(steps))
        , match_count_(0) {}
    
    double match(const LogEvent& event, 
                const std::deque<LogEvent>& context) override;
    
    void train(const LogEvent& event) override;
    
    std::string name() const override { return name_; }
    std::string description() const override;
    
    // Statistics
    uint64_t match_count() const { return match_count_; }
    
private:
    bool matches_step(const LogEvent& event, const Step& step) const;
    
    std::string name_;
    std::vector<Step> steps_;
    uint64_t match_count_;
    mutable std::mutex mutex_;
};

/**
 * @brief Detects unusual frequency patterns
 * 
 * Examples:
 * - Sudden burst of errors from same user
 * - Repeated failed login attempts
 * - High frequency of specific event type
 */
class FrequencyPattern : public PatternMatcher {
public:
    enum class FrequencyType {
        BURST,       // Sudden spike in event frequency
        REPEATED,    // Same entity appearing too frequently
        ABSENCE      // Expected event not occurring
    };
    
    FrequencyPattern(std::string name, 
                    std::string event_type,
                    FrequencyType type,
                    size_t threshold,
                    duration_t window = std::chrono::seconds(60))
        : name_(std::move(name))
        , event_type_(std::move(event_type))
        , type_(type)
        , threshold_(threshold)
        , window_(window) {}
    
    double match(const LogEvent& event, 
                const std::deque<LogEvent>& context) override;
    
    void train(const LogEvent& event) override;
    
    std::string name() const override { return name_; }
    std::string description() const override;
    
private:
    std::string name_;
    std::string event_type_;
    FrequencyType type_;
    size_t threshold_;
    duration_t window_;
    
    // Track event history
    std::deque<timestamp_t> event_times_;
    std::unordered_map<std::string, std::deque<timestamp_t>> entity_times_;
    mutable std::mutex mutex_;
};

/**
 * @brief Detects regex-based patterns in event messages or entities
 */
class RegexPattern : public PatternMatcher {
public:
    RegexPattern(std::string name, 
                std::string pattern,
                std::string field = "message")
        : name_(std::move(name))
        , pattern_(pattern)
        , field_(std::move(field))
        , regex_(pattern) {}
    
    double match(const LogEvent& event, 
                const std::deque<LogEvent>& context) override;
    
    void train(const LogEvent& event) override {}
    
    std::string name() const override { return name_; }
    std::string description() const override;
    
private:
    std::string name_;
    std::string pattern_;
    std::string field_;
    std::regex regex_;
};

/**
 * @brief Pattern library for managing and matching patterns
 */
class PatternEngine {
public:
    PatternEngine() = default;
    
    /**
     * @brief Register a pattern matcher
     */
    void register_pattern(std::shared_ptr<PatternMatcher> pattern);
    
    /**
     * @brief Check event against all patterns
     * @return Vector of matched patterns with scores
     */
    struct PatternMatch {
        std::shared_ptr<PatternMatcher> pattern;
        double score;
        std::string description;
    };
    
    std::vector<PatternMatch> match_patterns(
        const LogEvent& event,
        const std::deque<LogEvent>& context
    );
    
    /**
     * @brief Train all patterns with event
     */
    void train_all(const LogEvent& event);
    
    /**
     * @brief Get all registered patterns
     */
    std::vector<std::shared_ptr<PatternMatcher>> patterns() const;
    
    /**
     * @brief Register common built-in patterns
     */
    void register_builtin_patterns();
    
private:
    std::vector<std::shared_ptr<PatternMatcher>> patterns_;
    mutable std::mutex mutex_;
};

/**
 * @brief Factory for creating common patterns
 */
class PatternFactory {
public:
    /**
     * @brief Create cascading failure pattern
     * Database slow → API timeout → User error
     */
    static std::shared_ptr<SequentialPattern> cascading_failure();
    
    /**
     * @brief Create authentication failure pattern
     * Multiple failed logins from same user/IP
     */
    static std::shared_ptr<FrequencyPattern> auth_failure_pattern();
    
    /**
     * @brief Create retry storm pattern
     * Repeated retries of same operation
     */
    static std::shared_ptr<FrequencyPattern> retry_storm();
    
    /**
     * @brief Create memory leak pattern
     * Gradual increase in memory usage
     */
    static std::shared_ptr<SequentialPattern> memory_leak_pattern();
    
    /**
     * @brief Create exception pattern
     * Detects stack traces and exceptions
     */
    static std::shared_ptr<RegexPattern> exception_pattern();
};

} // namespace agentlog
