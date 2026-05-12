/**
 * @file apiary_logger_debug.cpp
 * @brief Rozszerzony moduł debugowania dla systemu APIARY Guard
 * 
 * Ten plik zawiera zaawansowane funkcje debugowania:
 * - Trace logging z pełnym kontekstem wykonania
 * - Performance monitoring z pomiarami czasu
 * - Memory leak detection helpers
 * - Stack trace utilities
 * - Debug assertions z logowaniem
 * - Thread activity monitoring
 * - Resource tracking
 * - Exception history
 * - Debug counters i metrics
 * 
 * KOMPILACJA:
 * g++ -std=c++17 -pthread -DDEBUG_BUILD -o apiary_logger_debug apiary_logger_debug.cpp apiary_logger.cpp
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#include "apiary_logger_debug.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <functional>
#include <memory>
#include <iomanip>
#include <set>
#include <condition_variable>

#ifdef DEBUG_BUILD

namespace apiary {
namespace debug {

// ============================================================================
// IMPLEMENTACJA DebugTracer
// ============================================================================

DebugTracer& DebugTracer::getInstance() {
    static DebugTracer instance;
    return instance;
}

DebugTracer::DebugTracer() : enabled_(true), depth_(0) {}

void DebugTracer::enable() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = true;
    if (Logger::getInstance().isInitialized()) {
        Logger::getInstance().debug("Debug tracer enabled", "DebugTracer");
    }
}

void DebugTracer::disable() {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = false;
    if (Logger::getInstance().isInitialized()) {
        Logger::getInstance().debug("Debug tracer disabled", "DebugTracer");
    }
}

void DebugTracer::traceEntry(const std::string& function, const std::string& file, int line) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    depth_++;
    
    auto now = std::chrono::steady_clock::now();
    traces_[function].push_back({TraceType::ENTRY, now, file, line, depth_});
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << std::string(depth_ * 2, ' ') << ">>> ENTER: " << function;
        Logger::getInstance().debug(oss.str(), "TRACE");
    }
}

void DebugTracer::traceExit(const std::string& function, const std::string& file, int line) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    traces_[function].push_back({TraceType::EXIT, now, file, line, depth_});
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << std::string(depth_ * 2, ' ') << "<<< EXIT: " << function;
        Logger::getInstance().debug(oss.str(), "TRACE");
    }
    
    if (depth_ > 0) depth_--;
}

void DebugTracer::tracePoint(const std::string& label, const std::string& file, int line) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    std::string context = "POINT_" + label;
    traces_[context].push_back({TraceType::POINT, now, file, line, depth_});
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << std::string(depth_ * 2, ' ') << "* POINT: " << label;
        Logger::getInstance().debug(oss.str(), "TRACE");
    }
}

std::vector<DebugTracer::TraceEntry> DebugTracer::getTraces(const std::string& function) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = traces_.find(function);
    if (it != traces_.end()) {
        return it->second;
    }
    return {};
}

void DebugTracer::clearTraces() {
    std::lock_guard<std::mutex> lock(mutex_);
    traces_.clear();
    depth_ = 0;
}

// ============================================================================
// IMPLEMENTACJA PerformanceMonitor
// ============================================================================

PerformanceMonitor& PerformanceMonitor::getInstance() {
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor() : enabled_(true) {}

void PerformanceMonitor::startTimer(const std::string& name) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    timers_[name] = std::chrono::steady_clock::now();
    
    if (Logger::getInstance().isInitialized()) {
        Logger::getInstance().debug("Timer started: " + name, "PERF");
    }
}

double PerformanceMonitor::stopTimer(const std::string& name) {
    if (!enabled_) return 0.0;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto end = std::chrono::steady_clock::now();
    
    auto it = timers_.find(name);
    if (it == timers_.end()) {
        if (Logger::getInstance().isInitialized()) {
            Logger::getInstance().warning("Timer not found: " + name, "PERF");
        }
        return 0.0;
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - it->second).count();
    double ms = duration / 1000.0;
    
    // Store in history
    history_[name].push_back(ms);
    if (history_[name].size() > max_history_size_) {
        history_[name].erase(history_[name].begin());
    }
    
    timers_.erase(it);
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << "Timer stopped: " << name << " = " << std::fixed 
            << std::setprecision(3) << ms << " ms";
        Logger::getInstance().info(oss.str(), "PERF");
    }
    
    return ms;
}

double PerformanceMonitor::getAverageTime(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = history_.find(name);
    if (it == history_.end() || it->second.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double val : it->second) {
        sum += val;
    }
    return sum / it->second.size();
}

void PerformanceMonitor::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    timers_.clear();
    history_.clear();
}

// ============================================================================
// IMPLEMENTACJA DebugCounter
// ============================================================================

DebugCounter& DebugCounter::getInstance() {
    static DebugCounter instance;
    return instance;
}

DebugCounter::DebugCounter() : verbose_(false) {}

void DebugCounter::increment(const std::string& name, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name] += value;
    
    if (Logger::getInstance().isInitialized() && verbose_) {
        std::ostringstream oss;
        oss << "Counter '" << name << "' = " << counters_[name];
        Logger::getInstance().debug(oss.str(), "COUNTER");
    }
}

int DebugCounter::get(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return counters_[name];
}

void DebugCounter::reset(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name] = 0;
}

void DebugCounter::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
}

std::map<std::string, int> DebugCounter::getAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    return counters_;
}

void DebugCounter::setVerbose(bool verbose) {
    verbose_ = verbose;
}

// ============================================================================
// IMPLEMENTACJA ResourceTracker
// ============================================================================

ResourceTracker& ResourceTracker::getInstance() {
    static ResourceTracker instance;
    return instance;
}

ResourceTracker::ResourceTracker() {}

void ResourceTracker::trackResource(const std::string& id, const std::string& type, 
                                    void* ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    resources_[id] = {type, ptr, std::chrono::steady_clock::now()};
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << "Resource tracked: " << id << " (" << type << ") @ " << ptr;
        Logger::getInstance().debug(oss.str(), "RESOURCE");
    }
}

void ResourceTracker::releaseResource(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second.created).count();
        
        if (Logger::getInstance().isInitialized()) {
            std::ostringstream oss;
            oss << "Resource released: " << id << " (" << it->second.type 
                << ") lifetime=" << duration << "ms";
            Logger::getInstance().debug(oss.str(), "RESOURCE");
        }
        
        resources_.erase(it);
    }
}

bool ResourceTracker::isTracked(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return resources_.find(id) != resources_.end();
}

std::vector<std::string> ResourceTracker::getLeakedResources() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> leaked;
    
    auto now = std::chrono::steady_clock::now();
    for (const auto& pair : resources_) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - pair.second.created).count();
        if (age > 300) { // 5 minutes threshold
            leaked.push_back(pair.first);
        }
    }
    
    return leaked;
}

void ResourceTracker::reportStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << "Resource status: " << resources_.size() << " active resources";
        Logger::getInstance().info(oss.str(), "RESOURCE");
        
        for (const auto& pair : resources_) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - pair.second.created).count();
            
            std::ostringstream res_oss;
            res_oss << "  - " << pair.first << " (" << pair.second.type 
                    << ") age=" << age << "s";
            Logger::getInstance().debug(res_oss.str(), "RESOURCE");
        }
    }
}

// ============================================================================
// IMPLEMENTACJA ExceptionHistory
// ============================================================================

ExceptionHistory& ExceptionHistory::getInstance() {
    static ExceptionHistory instance;
    return instance;
}

ExceptionHistory::ExceptionHistory() {}

void ExceptionHistory::record(const std::exception& e, const std::string& location) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ExceptionRecord record;
    record.message = e.what();
    record.location = location;
    record.timestamp = std::chrono::system_clock::now();
    record.type = typeid(e).name();
    
    history_.push_back(record);
    
    // Keep only last N exceptions
    if (history_.size() > max_history_size_) {
        history_.erase(history_.begin());
    }
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << "Exception recorded: " << e.what() << " at " << location;
        Logger::getInstance().error(oss.str(), "EXCEPTION");
    }
}

std::vector<ExceptionHistory::ExceptionRecord> ExceptionHistory::getHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

void ExceptionHistory::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

void ExceptionHistory::report() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (Logger::getInstance().isInitialized()) {
        std::ostringstream oss;
        oss << "Exception history: " << history_.size() << " exceptions recorded";
        Logger::getInstance().info(oss.str(), "EXCEPTION");
        
        for (const auto& record : history_) {
            auto time_t = std::chrono::system_clock::to_time_t(record.timestamp);
            std::ostringstream rec_oss;
            rec_oss << "  [" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                    << "] " << record.type << ": " << record.message 
                    << " at " << record.location;
            Logger::getInstance().warning(rec_oss.str(), "EXCEPTION");
        }
    }
}

// ============================================================================
// FREE FUNCTIONS
// ============================================================================

void debug_trace_entry(const std::string& func, const std::string& file, int line) {
    DebugTracer::getInstance().traceEntry(func, file, line);
}

void debug_trace_exit(const std::string& func, const std::string& file, int line) {
    DebugTracer::getInstance().traceExit(func, file, line);
}

void debug_trace_point(const std::string& label, const std::string& file, int line) {
    DebugTracer::getInstance().tracePoint(label, file, line);
}

void debug_start_timer(const std::string& name) {
    PerformanceMonitor::getInstance().startTimer(name);
}

double debug_stop_timer(const std::string& name) {
    return PerformanceMonitor::getInstance().stopTimer(name);
}

void debug_counter_inc(const std::string& name, int value) {
    DebugCounter::getInstance().increment(name, value);
}

void debug_counter_dec(const std::string& name, int value) {
    DebugCounter::getInstance().decrement(name, value);
}

int debug_counter_get(const std::string& name) {
    return DebugCounter::getInstance().get(name);
}

void debug_track_resource(const std::string& id, const std::string& type, void* ptr) {
    ResourceTracker::getInstance().trackResource(id, type, ptr);
}

void debug_release_resource(const std::string& id) {
    ResourceTracker::getInstance().releaseResource(id);
}

void debug_record_exception(const std::exception& e, const std::string& location) {
    ExceptionHistory::getInstance().record(e, location);
}

// RAII helper for automatic tracing
ScopedTracer::ScopedTracer(const std::string& func, const std::string& file, int line)
    : func_(func), file_(file), line_(line) {
    DebugTracer::getInstance().traceEntry(func_, file_, line_);
}

ScopedTracer::~ScopedTracer() {
    DebugTracer::getInstance().traceExit(func_, file_, line_);
}

ScopedTimer::ScopedTimer(const std::string& name) : name_(name) {
    PerformanceMonitor::getInstance().startTimer(name_);
}

ScopedTimer::~ScopedTimer() {
    PerformanceMonitor::getInstance().stopTimer(name_);
}

} // namespace debug
} // namespace apiary

#endif // DEBUG_BUILD
