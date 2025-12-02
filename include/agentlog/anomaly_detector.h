// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "common.h"
#include "event.h"
#include <deque>
#include <unordered_map>
#include <mutex>
#include <cmath>

namespace agentlog {

/**
 * @brief Base class for anomaly detection algorithms
 */
class AnomalyDetector {
public:
    virtual ~AnomalyDetector() = default;
    
    /**
     * @brief Calculate anomaly score for an event
     * @param event The log event to analyze
     * @return Anomaly score between 0.0 (normal) and 1.0 (highly anomalous)
     */
    virtual double score(const LogEvent& event) = 0;
    
    /**
     * @brief Train/update the detector with new data
     * @param event Training event
     */
    virtual void train(const LogEvent& event) = 0;
    
    /**
     * @brief Get detector name
     */
    virtual std::string name() const = 0;
};

/**
 * @brief Statistical anomaly detector using Z-score
 * 
 * Detects anomalies by comparing metric values to learned mean and stddev.
 * Uses Welford's online algorithm for numerical stability.
 */
class ZScoreDetector : public AnomalyDetector {
public:
    explicit ZScoreDetector(double threshold = 3.0)
        : threshold_(threshold) {}
    
    double score(const LogEvent& event) override;
    void train(const LogEvent& event) override;
    std::string name() const override { return "z_score"; }
    
private:
    struct Stats {
        double mean{0.0};
        double m2{0.0};  // Sum of squared differences
        uint64_t count{0};
        
        double stddev() const {
            return count > 1 ? std::sqrt(m2 / (count - 1)) : 0.0;
        }
    };
    
    std::unordered_map<std::string, Stats> metric_stats_;
    double threshold_;
    mutable std::mutex mutex_;
};

/**
 * @brief Moving average anomaly detector
 * 
 * Detects sudden spikes or drops compared to recent history.
 */
class MovingAverageDetector : public AnomalyDetector {
public:
    explicit MovingAverageDetector(size_t window_size = 100, double threshold = 2.5)
        : window_size_(window_size)
        , threshold_(threshold) {}
    
    double score(const LogEvent& event) override;
    void train(const LogEvent& event) override;
    std::string name() const override { return "moving_average"; }
    
private:
    struct History {
        std::deque<double> values;
        double sum{0.0};
    };
    
    std::unordered_map<std::string, History> metric_history_;
    size_t window_size_;
    double threshold_;
    mutable std::mutex mutex_;
};

/**
 * @brief Rate-based anomaly detector
 * 
 * Detects anomalous event rates (e.g., error rate spike).
 */
class RateDetector : public AnomalyDetector {
public:
    explicit RateDetector(std::chrono::seconds window = std::chrono::seconds(60))
        : window_duration_(window) {}
    
    double score(const LogEvent& event) override;
    void train(const LogEvent& event) override;
    std::string name() const override { return "rate"; }
    
private:
    struct RateStats {
        std::deque<timestamp_t> timestamps;
        double baseline_rate{0.0};
    };
    
    std::unordered_map<std::string, RateStats> event_rates_;
    duration_t window_duration_;
    mutable std::mutex mutex_;
};

/**
 * @brief Ensemble detector combining multiple algorithms
 * 
 * Uses voting or max score from multiple detectors for robust detection.
 */
class EnsembleDetector : public AnomalyDetector {
public:
    enum class CombineMethod {
        MAX,           // Take maximum score
        AVERAGE,       // Average of all scores
        WEIGHTED,      // Weighted average
        VOTING         // Count detectors above threshold
    };
    
    explicit EnsembleDetector(CombineMethod method = CombineMethod::MAX)
        : method_(method) {}
    
    void add_detector(std::shared_ptr<AnomalyDetector> detector, double weight = 1.0);
    
    double score(const LogEvent& event) override;
    void train(const LogEvent& event) override;
    std::string name() const override { return "ensemble"; }
    
private:
    struct DetectorInfo {
        std::shared_ptr<AnomalyDetector> detector;
        double weight;
    };
    
    std::vector<DetectorInfo> detectors_;
    CombineMethod method_;
};

/**
 * @brief Factory for creating detectors
 */
class DetectorFactory {
public:
    static std::shared_ptr<AnomalyDetector> create_default();
    static std::shared_ptr<AnomalyDetector> create_z_score(double threshold = 3.0);
    static std::shared_ptr<AnomalyDetector> create_moving_average(size_t window = 100);
    static std::shared_ptr<AnomalyDetector> create_rate(std::chrono::seconds window = std::chrono::seconds(60));
    static std::shared_ptr<AnomalyDetector> create_ensemble();
};

} // namespace agentlog
