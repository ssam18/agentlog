// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file agentlog.h
 * @brief Main header file for AgentLog - AI-Powered Logging Library
 * 
 * AgentLog is an intelligent logging framework that combines traditional
 * logging with AI-driven continuous analysis, enabling autonomous detection,
 * diagnosis, and response to application issues.
 * 
 * Key Features:
 * - Structured semantic events with rich metadata
 * - Real-time anomaly detection using multiple ML algorithms
 * - Pattern recognition and correlation across services
 * - Predictive analytics and trend forecasting
 * - Automated incident management with ticket creation
 * - OpenTelemetry integration for unified observability
 * 
 * Example Usage:
 * 
 * @code
 * #include <agentlog/agentlog.h>
 * 
 * int main() {
 *     // Initialize
 *     agentlog::Config config;
 *     config.service_name = "payment-service";
 *     config.enable_anomaly_detection = true;
 *     agentlog::global::init(config);
 *     
 *     // Structured logging
 *     AGENTLOG_EVENT("payment.processed")
 *         .entity("order_id", order_id)
 *         .metric("amount_usd", 99.99)
 *         .metric("processing_time_ms", 150.0)
 *         .severity(agentlog::Severity::INFO)
 *         .emit();
 *     
 *     // Observe metrics (automatic anomaly detection)
 *     AGENTLOG_OBSERVE("api.latency")
 *         .metric("latency_ms", latency)
 *         .context("endpoint", "/api/checkout")
 *         .emit();
 *     
 *     agentlog::global::shutdown();
 *     return 0;
 * }
 * @endcode
 */

// Core components
#include "agentlog/common.h"
#include "agentlog/event.h"
#include "agentlog/logger.h"

// AI/ML components
#include "agentlog/anomaly_detector.h"

// Optional components (include if needed)
// #include "agentlog/pattern_engine.h"
// #include "agentlog/correlation_engine.h"
// #include "agentlog/incident_manager.h"

// Version information
#define AGENTLOG_VERSION_MAJOR 0
#define AGENTLOG_VERSION_MINOR 1
#define AGENTLOG_VERSION_PATCH 0
#define AGENTLOG_VERSION_STRING "0.1.0"

// Make version available in source files
namespace agentlog {
    namespace detail {
        constexpr const char* version_string = AGENTLOG_VERSION_STRING;
    }
}

namespace agentlog {

/**
 * @brief Get AgentLog version string
 */
inline const char* version() {
    return AGENTLOG_VERSION_STRING;
}

/**
 * @brief Quick start initialization with defaults
 * 
 * @param service_name Name of your service/application
 * @param enable_ai Enable AI features (anomaly detection, patterns, etc.)
 */
inline void quick_init(const std::string& service_name, bool enable_ai = true) {
    Config config;
    config.service_name = service_name;
    config.enable_anomaly_detection = enable_ai;
    config.enable_pattern_matching = enable_ai;
    config.enable_correlation = enable_ai;
    global::init(config);
}

} // namespace agentlog
