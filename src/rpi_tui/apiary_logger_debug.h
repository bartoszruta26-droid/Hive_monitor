/**
 * @file apiary_logger_debug.h
 * @brief Nagłówek rozszerzonego modułu debugowania dla systemu APIARY Guard
 * 
 * Deklaracje klas i funkcji do zaawansowanego debugowania:
 * - DebugTracer: Śledzenie wejść/wyjść z funkcji
 * - PerformanceMonitor: Pomiar czasu wykonania
 * - DebugCounter: Liczniki debugowe
 * - ResourceTracker: Śledzenie zasobów
 * - ExceptionHistory: Historia wyjątków
 * - RAII helpers: ScopedTracer, ScopedTimer
 * 
 * UŻYCIE:
 * #define DEBUG_BUILD przed includowaniem tego pliku
 * 
 * Przykład:
 *   DEBUG_TRACE_FUNC(); // Na początku funkcji
 *   DEBUG_START_TIMER("operation");
 *   // ... kod ...
 *   DEBUG_STOP_TIMER("operation");
 *   DEBUG_COUNTER_INC("operations");
 */

#ifndef APIARY_LOGGER_DEBUG_H
#define APIARY_LOGGER_DEBUG_H

#include "apiary_logger.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <atomic>

#ifdef DEBUG_BUILD

namespace apiary {
namespace debug {

// ============================================================================
// DebugTracer - Śledzenie przepływu wykonania
// ============================================================================

enum class TraceType {
    ENTRY,      // Wejście do funkcji
    EXIT,       // Wyjście z funkcji
    POINT       // Punkt kontrolny
};

class DebugTracer {
public:
    struct TraceEntry {
        TraceType type;
        std::chrono::steady_clock::time_point timestamp;
        std::string file;
        int line;
        int depth;
    };
    
    static DebugTracer& getInstance();
    
    void enable();
    void disable();
    bool isEnabled() const { return enabled_; }
    
    void traceEntry(const std::string& function, const std::string& file, int line);
    void traceExit(const std::string& function, const std::string& file, int line);
    void tracePoint(const std::string& label, const std::string& file, int line);
    
    std::vector<TraceEntry> getTraces(const std::string& function);
    void clearTraces();
    
private:
    DebugTracer();
    DebugTracer(const DebugTracer&) = delete;
    DebugTracer& operator=(const DebugTracer&) = delete;
    
    mutable std::mutex mutex_;
    std::atomic<bool> enabled_;
    int depth_;
    std::map<std::string, std::vector<TraceEntry>> traces_;
};

// ============================================================================
// PerformanceMonitor - Monitorowanie wydajności
// ============================================================================

class PerformanceMonitor {
public:
    static PerformanceMonitor& getInstance();
    
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    
    void startTimer(const std::string& name);
    double stopTimer(const std::string& name);
    double getAverageTime(const std::string& name);
    void reset();
    
    size_t getMaxHistorySize() const { return max_history_size_; }
    void setMaxHistorySize(size_t size) { max_history_size_ = size; }
    
private:
    PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    mutable std::mutex mutex_;
    std::atomic<bool> enabled_;
    std::map<std::string, std::chrono::steady_clock::time_point> timers_;
    std::map<std::string, std::vector<double>> history_;
    size_t max_history_size_ = 1000;
};

// ============================================================================
// DebugCounter - Liczniki debugowe
// ============================================================================

class DebugCounter {
public:
    static DebugCounter& getInstance();
    
    void increment(const std::string& name, int value = 1);
    void decrement(const std::string& name, int value = 1) { increment(name, -value); }
    int get(const std::string& name);
    void reset(const std::string& name);
    void resetAll();
    std::map<std::string, int> getAll();
    
    void setVerbose(bool verbose);
    bool isVerbose() const { return verbose_; }
    
private:
    DebugCounter();
    DebugCounter(const DebugCounter&) = delete;
    DebugCounter& operator=(const DebugCounter&) = delete;
    
    mutable std::mutex mutex_;
    std::map<std::string, int> counters_;
    bool verbose_ = false;
};

// ============================================================================
// ResourceTracker - Śledzenie zasobów
// ============================================================================

class ResourceTracker {
public:
    struct ResourceInfo {
        std::string type;
        void* ptr;
        std::chrono::steady_clock::time_point created;
    };
    
    static ResourceTracker& getInstance();
    
    void trackResource(const std::string& id, const std::string& type, void* ptr);
    void releaseResource(const std::string& id);
    bool isTracked(const std::string& id);
    std::vector<std::string> getLeakedResources();
    void reportStatus();
    
private:
    ResourceTracker();
    ResourceTracker(const ResourceTracker&) = delete;
    ResourceTracker& operator=(const ResourceTracker&) = delete;
    
    mutable std::mutex mutex_;
    std::map<std::string, ResourceInfo> resources_;
};

// ============================================================================
// ExceptionHistory - Historia wyjątków
// ============================================================================

class ExceptionHistory {
public:
    struct ExceptionRecord {
        std::string message;
        std::string location;
        std::string type;
        std::chrono::system_clock::time_point timestamp;
    };
    
    static ExceptionHistory& getInstance();
    
    void record(const std::exception& e, const std::string& location);
    std::vector<ExceptionRecord> getHistory();
    void clear();
    void report();
    
    size_t getMaxHistorySize() const { return max_history_size_; }
    void setMaxHistorySize(size_t size) { max_history_size_ = size; }
    
private:
    ExceptionHistory();
    ExceptionHistory(const ExceptionHistory&) = delete;
    ExceptionHistory& operator=(const ExceptionHistory&) = delete;
    
    mutable std::mutex mutex_;
    std::vector<ExceptionRecord> history_;
    size_t max_history_size_ = 100;
};

// ============================================================================
// Free Functions - Łatwy dostęp do funkcjonalności
// ============================================================================

void debug_trace_entry(const std::string& func, const std::string& file, int line);
void debug_trace_exit(const std::string& func, const std::string& file, int line);
void debug_trace_point(const std::string& label, const std::string& file, int line);

void debug_start_timer(const std::string& name);
double debug_stop_timer(const std::string& name);

void debug_counter_inc(const std::string& name, int value = 1);
void debug_counter_dec(const std::string& name, int value = 1);
int debug_counter_get(const std::string& name);

void debug_track_resource(const std::string& id, const std::string& type, void* ptr);
void debug_release_resource(const std::string& id);

void debug_record_exception(const std::exception& e, const std::string& location);

// ============================================================================
// RAII Helpers - Automatyczne zarządzanie
// ============================================================================

class ScopedTracer {
public:
    ScopedTracer(const std::string& func, const std::string& file, int line);
    ~ScopedTracer();
    
private:
    std::string func_;
    std::string file_;
    int line_;
};

class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name);
    ~ScopedTimer();
    
private:
    std::string name_;
};

} // namespace debug
} // namespace apiary

// ============================================================================
// Makra pomocnicze
// ============================================================================

#define DEBUG_TRACE_FUNC() \
    apiary::debug::ScopedTracer __tracer__(__func__, __FILE__, __LINE__)

#define DEBUG_TRACE_POINT(label) \
    apiary::debug::debug_trace_point(label, __FILE__, __LINE__)

#define DEBUG_START_TIMER(name) \
    apiary::debug::debug_start_timer(name)

#define DEBUG_STOP_TIMER(name) \
    apiary::debug::debug_stop_timer(name)

#define DEBUG_SCOPED_TIMER(name) \
    apiary::debug::ScopedTimer __timer__(name)

#define DEBUG_COUNTER_INC(name) \
    apiary::debug::debug_counter_inc(name, 1)

#define DEBUG_COUNTER_DEC(name, val) \
    apiary::debug::debug_counter_dec(name, val)

#define DEBUG_COUNTER_GET(name) \
    apiary::debug::debug_counter_get(name)

#define DEBUG_TRACK_RESOURCE(id, type, ptr) \
    apiary::debug::debug_track_resource(id, type, ptr)

#define DEBUG_RELEASE_RESOURCE(id) \
    apiary::debug::debug_release_resource(id)

#define DEBUG_RECORD_EXCEPTION(e) \
    apiary::debug::debug_record_exception(e, std::string(__func__) + " at " + __FILE__)

#else // DEBUG_BUILD not defined

// Stub implementations when DEBUG_BUILD is not defined
#define DEBUG_TRACE_FUNC()
#define DEBUG_TRACE_POINT(label)
#define DEBUG_START_TIMER(name)
#define DEBUG_STOP_TIMER(name)
#define DEBUG_SCOPED_TIMER(name)
#define DEBUG_COUNTER_INC(name)
#define DEBUG_COUNTER_DEC(name, val)
#define DEBUG_COUNTER_GET(name) (0)
#define DEBUG_TRACK_RESOURCE(id, type, ptr)
#define DEBUG_RELEASE_RESOURCE(id)
#define DEBUG_RECORD_EXCEPTION(e)

#endif // DEBUG_BUILD

#endif // APIARY_LOGGER_DEBUG_H
