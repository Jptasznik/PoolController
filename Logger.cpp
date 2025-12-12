#include "Logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdarg.h>

// ============================================
// INTERNAL STATE
// ============================================

static LogLevel currentLogLevel = LOG_INFO;  // Default: show INFO and above
static bool timestampsEnabled = true;
static bool colorsEnabled = false;  // Disabled by default (not all terminals support)
static SemaphoreHandle_t logMutex = NULL;

// Maximum number of custom log handlers
#define MAX_LOG_HANDLERS 4
static LogHandler logHandlers[MAX_LOG_HANDLERS] = {NULL};
static int logHandlerCount = 0;

// Buffer for formatted messages
#define LOG_BUFFER_SIZE 256
static char logBuffer[LOG_BUFFER_SIZE];

// ============================================
// ANSI COLOR CODES (for terminal support)
// ============================================

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GRAY    "\033[90m"

// ============================================
// INITIALIZATION
// ============================================

void initLogger() {
  // Create mutex for thread-safe logging
  if (logMutex == NULL) {
    logMutex = xSemaphoreCreateMutex();
    if (logMutex == NULL) {
      Serial.println("ERROR: Failed to create log mutex!");
    }
  }

  LOG_INFO("Logging system initialized");
}

// ============================================
// CONFIGURATION
// ============================================

void setLogLevel(LogLevel level) {
  currentLogLevel = level;
  LOG_INFOF("Log level set to: %s", logLevelToString(level));
}

LogLevel getLogLevel() {
  return currentLogLevel;
}

void setLogTimestamps(bool enabled) {
  timestampsEnabled = enabled;
}

void setLogColors(bool enabled) {
  colorsEnabled = enabled;
}

// ============================================
// CORE LOGGING FUNCTIONS
// ============================================

void log(LogLevel level, const char* module, const char* message) {
  // Filter by log level
  if (level < currentLogLevel) {
    return;
  }

  // Thread-safe logging
  if (logMutex != NULL) {
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
      // Couldn't get mutex, skip this log message
      return;
    }
  }

  unsigned long timestamp = millis();

  // Format and output to Serial
  if (colorsEnabled) {
    const char* color = COLOR_RESET;
    switch (level) {
      case LOG_DEBUG: color = COLOR_GRAY; break;
      case LOG_INFO:  color = COLOR_RESET; break;
      case LOG_WARN:  color = COLOR_YELLOW; break;
      case LOG_ERROR: color = COLOR_RED; break;
    }
    Serial.print(color);
  }

  if (timestampsEnabled) {
    Serial.print("[");
    Serial.print(timestamp);
    Serial.print("] ");
  }

  Serial.print("[");
  Serial.print(logLevelToShortString(level));
  Serial.print("] ");

  Serial.print("[");
  Serial.print(module);
  Serial.print("] ");

  Serial.println(message);

  if (colorsEnabled) {
    Serial.print(COLOR_RESET);
  }

  // Call custom log handlers
  for (int i = 0; i < logHandlerCount; i++) {
    if (logHandlers[i] != NULL) {
      logHandlers[i](level, module, message, timestamp);
    }
  }

  // Release mutex
  if (logMutex != NULL) {
    xSemaphoreGive(logMutex);
  }
}

void logf(LogLevel level, const char* module, const char* format, ...) {
  // Filter by log level
  if (level < currentLogLevel) {
    return;
  }

  // Format the message
  va_list args;
  va_start(args, format);
  vsnprintf(logBuffer, LOG_BUFFER_SIZE, format, args);
  va_end(args);

  // Use the regular log function
  log(level, module, logBuffer);
}

// ============================================
// EXTENSIBILITY
// ============================================

void addLogHandler(LogHandler handler) {
  if (handler == NULL) return;

  // Thread-safe handler management
  if (logMutex != NULL) {
    xSemaphoreTake(logMutex, portMAX_DELAY);
  }

  if (logHandlerCount < MAX_LOG_HANDLERS) {
    logHandlers[logHandlerCount++] = handler;
    Serial.print("Added custom log handler (total: ");
    Serial.print(logHandlerCount);
    Serial.println(")");
  } else {
    Serial.println("ERROR: Maximum log handlers reached!");
  }

  if (logMutex != NULL) {
    xSemaphoreGive(logMutex);
  }
}

void removeLogHandler(LogHandler handler) {
  if (handler == NULL) return;

  // Thread-safe handler management
  if (logMutex != NULL) {
    xSemaphoreTake(logMutex, portMAX_DELAY);
  }

  // Find and remove handler
  for (int i = 0; i < logHandlerCount; i++) {
    if (logHandlers[i] == handler) {
      // Shift remaining handlers
      for (int j = i; j < logHandlerCount - 1; j++) {
        logHandlers[j] = logHandlers[j + 1];
      }
      logHandlers[--logHandlerCount] = NULL;
      Serial.println("Removed custom log handler");
      break;
    }
  }

  if (logMutex != NULL) {
    xSemaphoreGive(logMutex);
  }
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

const char* logLevelToString(LogLevel level) {
  switch (level) {
    case LOG_DEBUG: return "DEBUG";
    case LOG_INFO:  return "INFO";
    case LOG_WARN:  return "WARN";
    case LOG_ERROR: return "ERROR";
    default:        return "UNKNOWN";
  }
}

const char* logLevelToShortString(LogLevel level) {
  switch (level) {
    case LOG_DEBUG: return "DBG";
    case LOG_INFO:  return "INF";
    case LOG_WARN:  return "WRN";
    case LOG_ERROR: return "ERR";
    default:        return "???";
  }
}

void flushLogs() {
  Serial.flush();
  // Future: flush other log destinations (files, network buffers, etc.)
}
