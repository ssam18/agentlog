// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace agentlog {

// Time types
using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;
using duration_t = std::chrono::nanoseconds;

// Severity levels
enum class Severity {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5,
    ALERT = 6  // AI-generated high-priority alert
};

inline const char* severity_to_string(Severity s) {
    switch (s) {
        case Severity::TRACE: return "TRACE";
        case Severity::DEBUG: return "DEBUG";
        case Severity::INFO: return "INFO";
        case Severity::WARNING: return "WARNING";
        case Severity::ERROR: return "ERROR";
        case Severity::CRITICAL: return "CRITICAL";
        case Severity::ALERT: return "ALERT";
        default: return "UNKNOWN";
    }
}

// Entity types for semantic understanding
enum class EntityType {
    USER,
    SERVICE,
    ENDPOINT,
    DATABASE,
    CACHE,
    QUEUE,
    FILE,
    NETWORK,
    CUSTOM
};

// Metric types
using MetricValue = double;
using MetricMap = std::map<std::string, MetricValue>;

// Context types
using ContextMap = std::map<std::string, std::string>;

// Stack trace frame
struct StackFrame {
    std::string function;
    std::string file;
    uint32_t line{0};
    std::string module;
    
    StackFrame() = default;
    StackFrame(std::string func, std::string f, uint32_t l)
        : function(std::move(func)), file(std::move(f)), line(l) {}
};

using StackTrace = std::vector<StackFrame>;

// Forward declarations
class LogEvent;
class Logger;
class AnomalyDetector;
class PatternEngine;
class CorrelationEngine;
class IncidentManager;

// Smart pointers
using LogEventPtr = std::shared_ptr<LogEvent>;
using LoggerPtr = std::shared_ptr<Logger>;
using AnomalyDetectorPtr = std::shared_ptr<AnomalyDetector>;
using PatternEnginePtr = std::shared_ptr<PatternEngine>;
using CorrelationEnginePtr = std::shared_ptr<CorrelationEngine>;
using IncidentManagerPtr = std::shared_ptr<IncidentManager>;

} // namespace agentlog
