/**
 * @file test_debug_main.cpp
 * @brief Testowy program główny dla modułu debugowania
 */

#define DEBUG_BUILD
#include "apiary_logger.h"
#include "apiary_logger_debug.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace apiary;
    
    // Inicjalizacja loggera
    LoggerConfig config;
    config.log_file = "/tmp/apiary_test.log";
    config.debug_file = "/tmp/apiary_test_debug.log";
    config.console_output = true;
    config.file_output = true;
    
    Logger::getInstance().initialize(config);
    
    std::cout << "=== Test Debug Module ===" << std::endl;
    
    // Test DebugTracer
    {
        DEBUG_TRACE_FUNC();
        Logger::getInstance().info("Testing DebugTracer", "TEST");
        DEBUG_TRACE_POINT("mid_function");
    }
    
    // Test PerformanceMonitor
    {
        DEBUG_START_TIMER("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        double elapsed = DEBUG_STOP_TIMER("test_operation");
        std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;
    }
    
    // Test DebugCounter
    {
        DEBUG_COUNTER_INC("operations");
        DEBUG_COUNTER_INC("operations");
        DEBUG_COUNTER_DEC("operations", 1);
        int count = DEBUG_COUNTER_GET("operations");
        std::cout << "Counter value: " << count << std::endl;
    }
    
    // Test ScopedTimer
    {
        DEBUG_SCOPED_TIMER("scoped_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Test ExceptionHistory
    try {
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        DEBUG_RECORD_EXCEPTION(e);
    }
    
    // Report status
    debug::ExceptionHistory::getInstance().report();
    debug::DebugCounter::getInstance().setVerbose(true);
    
    Logger::getInstance().shutdown();
    
    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
