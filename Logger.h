#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// ============================================
// LOGGING FRAMEWORK
// ============================================
// Provides structured logging with multiple severity levels.
// Initial implementation uses Serial output, but designed to be
// extensible for future enhancements like:
// - Flash storage logging
// - Cloud/network logging
// - Log rotation and buffering
// - Multiple simultaneous outputs

// ============================================
// LOG LEVELS
// ============================================

enum LogLevel {
  LOG_DEBUG = 0,  // Detailed debugging information
  LOG_INFO = 1,   // General informational messages
  LOG_WARN = 2,   // Warning messages (potential issues)
  LOG_ERROR = 3   // Error messages (serious problems)
};

// ============================================
// LOGGER CONFIGURATION
// ============================================

// Initialize the logging system
void initLogger();

// Set minimum log level (messages below this level are filtered out)
void setLogLevel(LogLevel level);

// Get current log level
LogLevel getLogLevel();

// Enable/disable timestamps in log output
void setLogTimestamps(bool enabled);

// Enable/disable colored output (if terminal supports it)
void setLogColors(bool enabled);

// ============================================
// LOGGING FUNCTIONS
// ============================================

// Core logging function (use macros below for convenience)
void log(LogLevel level, const char* module, const char* message);
void logf(LogLevel level, const char* module, const char* format, ...);

// ============================================
// CONVENIENCE MACROS
// ============================================
// These macros automatically include the current module name
// and provide a clean interface for logging

// Define module name for logging (add to top of each .cpp file)
#ifndef LOG_MODULE
#define LOG_MODULE "SYSTEM"
#endif

// Logging macros - use these in your code
#define LOG_DEBUG(msg) log(LOG_DEBUG, LOG_MODULE, msg)
#define LOG_INFO(msg) log(LOG_INFO, LOG_MODULE, msg)
#define LOG_WARN(msg) log(LOG_WARN, LOG_MODULE, msg)
#define LOG_ERROR(msg) log(LOG_ERROR, LOG_MODULE, msg)

// Formatted logging macros (printf-style)
#define LOG_DEBUGF(fmt, ...) logf(LOG_DEBUG, LOG_MODULE, fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) logf(LOG_INFO, LOG_MODULE, fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) logf(LOG_WARN, LOG_MODULE, fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) logf(LOG_ERROR, LOG_MODULE, fmt, ##__VA_ARGS__)

// ============================================
// EXTENSIBILITY INTERFACE
// ============================================
// For future enhancement: Add custom log handlers

// Log handler function type
typedef void (*LogHandler)(LogLevel level, const char* module, const char* message, unsigned long timestamp);

// Add a custom log handler (future: multiple outputs)
void addLogHandler(LogHandler handler);

// Remove a log handler
void removeLogHandler(LogHandler handler);

// ============================================
// UTILITY FUNCTIONS
// ============================================

// Convert log level to string
const char* logLevelToString(LogLevel level);

// Convert log level to short string (for compact output)
const char* logLevelToShortString(LogLevel level);

// Flush all log outputs (ensure everything is written)
void flushLogs();

#endif // LOGGER_H
