// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/event.h"
#include "agentlog/logger.h"
#include <sstream>
#include <iomanip>

#ifdef __GNUC__
#include <execinfo.h>
#include <cxxabi.h>
#endif

namespace agentlog {

LogEvent& LogEvent::capture_stack_trace(size_t max_frames) {
#ifdef __GNUC__
    void* buffer[256];
    int frames = backtrace(buffer, std::min(max_frames, size_t(256)));
    char** symbols = backtrace_symbols(buffer, frames);
    
    if (symbols) {
        for (int i = 0; i < frames; ++i) {
            std::string symbol = symbols[i];
            
            // Parse symbol: module(function+offset) [address]
            StackFrame frame;
            
            size_t paren_start = symbol.find('(');
            size_t paren_end = symbol.find('+');
            
            if (paren_start != std::string::npos && paren_end != std::string::npos) {
                frame.module = symbol.substr(0, paren_start);
                std::string mangled = symbol.substr(paren_start + 1, paren_end - paren_start - 1);
                
                // Demangle C++ names
                int status;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    frame.function = demangled;
                    free(demangled);
                } else {
                    frame.function = mangled;
                }
            } else {
                frame.function = symbol;
            }
            
            stack_trace_.push_back(frame);
        }
        free(symbols);
    }
#endif
    return *this;
}

std::string LogEvent::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"event_id\":" << event_id_ << ",";
    oss << "\"event_type\":\"" << event_type_ << "\",";
    oss << "\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp_.time_since_epoch()).count() << ",";
    oss << "\"severity\":\"" << severity_to_string(severity_) << "\",";
    
    if (!message_.empty()) {
        oss << "\"message\":\"" << message_ << "\",";
    }
    
    if (!service_name_.empty()) {
        oss << "\"service\":\"" << service_name_ << "\",";
    }
    
    if (!trace_id_.empty()) {
        oss << "\"trace_id\":\"" << trace_id_ << "\",";
    }
    
    // Entities
    if (!entities_.empty()) {
        oss << "\"entities\":{";
        bool first = true;
        for (const auto& [key, value] : entities_) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        oss << "},";
    }
    
    // Metrics
    if (!metrics_.empty()) {
        oss << "\"metrics\":{";
        bool first = true;
        for (const auto& [key, value] : metrics_) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        oss << "},";
    }
    
    // Context
    if (!context_.empty()) {
        oss << "\"context\":{";
        bool first = true;
        for (const auto& [key, value] : context_) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        oss << "},";
    }
    
    // AI fields
    oss << "\"anomaly_score\":" << anomaly_score_;
    
    if (incident_id_) {
        oss << ",\"incident_id\":\"" << *incident_id_ << "\"";
    }
    
    oss << "}";
    return oss.str();
}

std::string LogEvent::to_string() const {
    std::ostringstream oss;
    
    // Timestamp
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    // Severity
    oss << " [" << severity_to_string(severity_) << "]";
    
    // Service
    if (!service_name_.empty()) {
        oss << " [" << service_name_;
        if (!service_instance_.empty()) {
            oss << ":" << service_instance_;
        }
        oss << "]";
    }
    
    // Event type
    oss << " " << event_type_;
    
    // Message
    if (!message_.empty()) {
        oss << " - " << message_;
    }
    
    // Entities
    if (!entities_.empty()) {
        oss << " {";
        bool first = true;
        for (const auto& [key, value] : entities_) {
            if (!first) oss << ", ";
            oss << key << "=" << value;
            first = false;
        }
        oss << "}";
    }
    
    // Metrics
    if (!metrics_.empty()) {
        oss << " [";
        bool first = true;
        for (const auto& [key, value] : metrics_) {
            if (!first) oss << ", ";
            oss << key << "=" << value;
            first = false;
        }
        oss << "]";
    }
    
    // Anomaly indicator
    if (is_anomalous()) {
        oss << " ⚠️ ANOMALY(" << anomaly_score_ << ")";
    }
    
    return oss.str();
}

void EventBuilder::emit() {
    Logger::instance().emit(event_);
}

} // namespace agentlog
