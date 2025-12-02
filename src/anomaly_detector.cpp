// Copyright (c) 2025 AgentLog Contributors
// SPDX-License-Identifier: MIT

#include "agentlog/anomaly_detector.h"
#include <algorithm>
#include <cmath>

namespace agentlog {

//=============================================================================
// ZScoreDetector Implementation
//=============================================================================

double ZScoreDetector::score(const LogEvent& event) {
    if (event.metrics().empty()) {
        return 0.0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    double max_zscore = 0.0;
    
    for (const auto& [metric_name, value] : event.metrics()) {
        auto it = metric_stats_.find(metric_name);
        if (it == metric_stats_.end() || it->second.count < 30) {
            // Not enough data yet
            continue;
        }
        
        const auto& stats = it->second;
        double stddev = stats.stddev();
        
        if (stddev < 1e-6) {
            // No variance - constant metric
            if (std::abs(value - stats.mean) > 1e-6) {
                return 1.0;  // Suddenly changed
            }
            continue;
        }
        
        // Calculate Z-score
        double zscore = std::abs(value - stats.mean) / stddev;
        
        // Normalize to 0-1 range using sigmoid-like function
        double normalized = std::tanh(zscore / threshold_);
        max_zscore = std::max(max_zscore, normalized);
    }
    
    return max_zscore;
}

void ZScoreDetector::train(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [metric_name, value] : event.metrics()) {
        auto& stats = metric_stats_[metric_name];
        
        // Welford's online algorithm for numerical stability
        stats.count++;
        double delta = value - stats.mean;
        stats.mean += delta / stats.count;
        double delta2 = value - stats.mean;
        stats.m2 += delta * delta2;
    }
}

//=============================================================================
// MovingAverageDetector Implementation
//=============================================================================

double MovingAverageDetector::score(const LogEvent& event) {
    if (event.metrics().empty()) {
        return 0.0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    double max_deviation = 0.0;
    
    for (const auto& [metric_name, value] : event.metrics()) {
        auto it = metric_history_.find(metric_name);
        if (it == metric_history_.end() || it->second.values.size() < 10) {
            // Not enough history
            continue;
        }
        
        const auto& history = it->second;
        double avg = history.sum / history.values.size();
        
        // Calculate MAD (Mean Absolute Deviation)
        double mad = 0.0;
        for (double v : history.values) {
            mad += std::abs(v - avg);
        }
        mad /= history.values.size();
        
        if (mad < 1e-6) {
            if (std::abs(value - avg) > 1e-6) {
                return 1.0;
            }
            continue;
        }
        
        // Deviation score
        double deviation = std::abs(value - avg) / (threshold_ * mad);
        max_deviation = std::max(max_deviation, std::tanh(deviation));
    }
    
    return max_deviation;
}

void MovingAverageDetector::train(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [metric_name, value] : event.metrics()) {
        auto& history = metric_history_[metric_name];
        
        history.values.push_back(value);
        history.sum += value;
        
        if (history.values.size() > window_size_) {
            history.sum -= history.values.front();
            history.values.pop_front();
        }
    }
}

//=============================================================================
// RateDetector Implementation
//=============================================================================

double RateDetector::score(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string& event_type = event.event_type();
    auto& rate_stats = event_rates_[event_type];
    
    if (rate_stats.timestamps.empty()) {
        return 0.0;
    }
    
    // Clean old timestamps
    auto cutoff = event.timestamp() - window_duration_;
    while (!rate_stats.timestamps.empty() && 
           rate_stats.timestamps.front() < cutoff) {
        rate_stats.timestamps.pop_front();
    }
    
    // Calculate current rate (events per second)
    double current_rate = static_cast<double>(rate_stats.timestamps.size()) / 
                         std::chrono::duration<double>(window_duration_).count();
    
    if (rate_stats.baseline_rate < 0.1) {
        // No baseline yet
        return 0.0;
    }
    
    // Compare to baseline
    double ratio = current_rate / rate_stats.baseline_rate;
    
    // Score based on deviation from baseline
    if (ratio > 2.0) {
        // Rate spike
        return std::min(1.0, (ratio - 2.0) / 3.0);
    } else if (ratio < 0.5) {
        // Rate drop
        return std::min(1.0, (0.5 - ratio) / 0.5);
    }
    
    return 0.0;
}

void RateDetector::train(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string& event_type = event.event_type();
    auto& rate_stats = event_rates_[event_type];
    
    rate_stats.timestamps.push_back(event.timestamp());
    
    // Update baseline (exponential moving average)
    auto cutoff = event.timestamp() - window_duration_;
    size_t count = 0;
    for (const auto& ts : rate_stats.timestamps) {
        if (ts >= cutoff) count++;
    }
    
    double current_rate = static_cast<double>(count) / 
                         std::chrono::duration<double>(window_duration_).count();
    
    if (rate_stats.baseline_rate < 0.1) {
        rate_stats.baseline_rate = current_rate;
    } else {
        // EMA with alpha = 0.1
        rate_stats.baseline_rate = 0.9 * rate_stats.baseline_rate + 0.1 * current_rate;
    }
}

//=============================================================================
// EnsembleDetector Implementation
//=============================================================================

void EnsembleDetector::add_detector(std::shared_ptr<AnomalyDetector> detector, double weight) {
    detectors_.push_back({std::move(detector), weight});
}

double EnsembleDetector::score(const LogEvent& event) {
    if (detectors_.empty()) {
        return 0.0;
    }
    
    std::vector<double> scores;
    scores.reserve(detectors_.size());
    
    for (const auto& info : detectors_) {
        double s = info.detector->score(event);
        scores.push_back(s);
    }
    
    switch (method_) {
        case CombineMethod::MAX:
            return *std::max_element(scores.begin(), scores.end());
            
        case CombineMethod::AVERAGE: {
            double sum = 0.0;
            for (double s : scores) sum += s;
            return sum / scores.size();
        }
        
        case CombineMethod::WEIGHTED: {
            double sum = 0.0;
            double weight_sum = 0.0;
            for (size_t i = 0; i < scores.size(); ++i) {
                sum += scores[i] * detectors_[i].weight;
                weight_sum += detectors_[i].weight;
            }
            return weight_sum > 0 ? sum / weight_sum : 0.0;
        }
        
        case CombineMethod::VOTING: {
            int votes = 0;
            for (double s : scores) {
                if (s >= 0.5) votes++;
            }
            return static_cast<double>(votes) / scores.size();
        }
    }
    
    return 0.0;
}

void EnsembleDetector::train(const LogEvent& event) {
    for (auto& info : detectors_) {
        info.detector->train(event);
    }
}

//=============================================================================
// DetectorFactory Implementation
//=============================================================================

std::shared_ptr<AnomalyDetector> DetectorFactory::create_default() {
    // Create ensemble with multiple detectors
    auto ensemble = std::make_shared<EnsembleDetector>(
        EnsembleDetector::CombineMethod::MAX
    );
    
    ensemble->add_detector(create_z_score(3.0), 1.0);
    ensemble->add_detector(create_moving_average(100), 1.0);
    ensemble->add_detector(create_rate(std::chrono::seconds(60)), 0.8);
    
    return ensemble;
}

std::shared_ptr<AnomalyDetector> DetectorFactory::create_z_score(double threshold) {
    return std::make_shared<ZScoreDetector>(threshold);
}

std::shared_ptr<AnomalyDetector> DetectorFactory::create_moving_average(size_t window) {
    return std::make_shared<MovingAverageDetector>(window);
}

std::shared_ptr<AnomalyDetector> DetectorFactory::create_rate(std::chrono::seconds window) {
    return std::make_shared<RateDetector>(window);
}

std::shared_ptr<AnomalyDetector> DetectorFactory::create_ensemble() {
    return create_default();
}

} // namespace agentlog
