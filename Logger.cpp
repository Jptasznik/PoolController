#include "Logger.h"
#include "Config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdarg.h>
#include <Preferences.h>

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

// NVS storage
static Preferences preferences;
static SemaphoreHandle_t nvsMutex = NULL;

// Circular log buffer configuration
#define LOG_ENTRY_MAX_SIZE 80   // Maximum size per log entry
static unsigned int logBufferMaxEntries = DEFAULT_LOG_BUFFER_SIZE;  // Dynamic buffer size (12-100)

// NVS configuration
#define NVS_NAMESPACE "logger"           // NVS namespace for logger
#define NVS_KEY_LOG_LEVEL "logLevel"     // NVS key for log level
#define NVS_KEY_LOG_BUF_SIZE "logBufSize" // NVS key for buffer size
#define NVS_KEY_LOG_INDEX "logIndex"     // NVS key for write index
#define NVS_KEY_LOG_PREFIX "log"         // NVS key prefix for log entries (log00, log01, etc.)
#define NVS_KEY_MAX_LENGTH 16            // Maximum length for NVS key strings

// Mutex timeout configuration (milliseconds)
#define LOG_MUTEX_TIMEOUT_MS 100         // Timeout for acquiring log mutex (for serial output)
#define NVS_MUTEX_TIMEOUT_MS 1000        // Timeout for acquiring NVS mutex (read/write operations)
#define NVS_WRITE_TIMEOUT_MS 10          // Non-blocking timeout for NVS writes in log() function

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

  // Create mutex for NVS access
  if (nvsMutex == NULL) {
    nvsMutex = xSemaphoreCreateMutex();
    if (nvsMutex == NULL) {
      Serial.println("ERROR: Failed to create NVS mutex!");
    }
  }

  // Load saved log level from NVS
  LogLevel savedLevel = loadLogLevel();
  if (savedLevel >= LOG_DEBUG && savedLevel <= LOG_ERROR) {
    currentLogLevel = savedLevel;
  }

  // Load saved log buffer size from NVS
  unsigned int savedBufferSize = loadLogBufferSize();
  if (savedBufferSize >= MIN_LOG_BUFFER_SIZE && savedBufferSize <= MAX_LOG_BUFFER_SIZE) {
    logBufferMaxEntries = savedBufferSize;
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
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(LOG_MUTEX_TIMEOUT_MS)) != pdTRUE) {
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

  // Write to NVS circular buffer (INFO level and above only, to save flash wear)
  if (level >= LOG_INFO) {
    // Format entry for NVS: "[timestamp] [LEVEL] [MODULE] message"
    char nvsEntry[LOG_ENTRY_MAX_SIZE];
    snprintf(nvsEntry, LOG_ENTRY_MAX_SIZE, "[%lu] [%s] [%s] %s",
             timestamp, logLevelToShortString(level), module, message);

    // Write to circular buffer (non-blocking, best effort)
    if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_WRITE_TIMEOUT_MS)) == pdTRUE) {
      preferences.begin(NVS_NAMESPACE, false);

      // Get current write index
      unsigned int writeIndex = preferences.getUInt(NVS_KEY_LOG_INDEX, 0);

      // Write entry
      char key[NVS_KEY_MAX_LENGTH];
      snprintf(key, sizeof(key), "%s%02u", NVS_KEY_LOG_PREFIX, writeIndex);
      preferences.putString(key, nvsEntry);

      // Update write index (circular)
      writeIndex = (writeIndex + 1) % logBufferMaxEntries;
      preferences.putUInt(NVS_KEY_LOG_INDEX, writeIndex);

      preferences.end();
      xSemaphoreGive(nvsMutex);
    }
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

// ============================================
// NVS PERSISTENCE
// ============================================

void saveLogLevel(LogLevel level) {
  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putUChar(NVS_KEY_LOG_LEVEL, (uint8_t)level);
    preferences.end();
    xSemaphoreGive(nvsMutex);
  }
}

LogLevel loadLogLevel() {
  LogLevel level = LOG_INFO;  // Default

  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, true);  // Read-only
    uint8_t storedLevel = preferences.getUChar(NVS_KEY_LOG_LEVEL, LOG_INFO);
    preferences.end();
    xSemaphoreGive(nvsMutex);

    // Validate stored value
    if (storedLevel >= LOG_DEBUG && storedLevel <= LOG_ERROR) {
      level = (LogLevel)storedLevel;
    }
  }

  return level;
}

void saveLogBufferSize(unsigned int size) {
  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putUInt(NVS_KEY_LOG_BUF_SIZE, size);
    preferences.end();
    xSemaphoreGive(nvsMutex);
  }
}

unsigned int loadLogBufferSize() {
  unsigned int size = DEFAULT_LOG_BUFFER_SIZE;  // Default

  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, true);  // Read-only
    unsigned int storedSize = preferences.getUInt(NVS_KEY_LOG_BUF_SIZE, DEFAULT_LOG_BUFFER_SIZE);
    preferences.end();
    xSemaphoreGive(nvsMutex);

    // Validate stored value
    if (storedSize >= MIN_LOG_BUFFER_SIZE && storedSize <= MAX_LOG_BUFFER_SIZE) {
      size = storedSize;
    }
  }

  return size;
}

unsigned int getLogBufferMaxEntries() {
  return logBufferMaxEntries;
}

void setLogBufferMaxEntries(unsigned int size) {
  // Validate size
  if (size < MIN_LOG_BUFFER_SIZE) {
    size = MIN_LOG_BUFFER_SIZE;
  } else if (size > MAX_LOG_BUFFER_SIZE) {
    size = MAX_LOG_BUFFER_SIZE;
  }

  logBufferMaxEntries = size;
}

// ============================================
// CIRCULAR LOG BUFFER (NVS)
// ============================================

void initLogBuffer() {
  // Create NVS mutex if not already created
  if (nvsMutex == NULL) {
    nvsMutex = xSemaphoreCreateMutex();
    if (nvsMutex == NULL) {
      Serial.println("ERROR: Failed to create NVS mutex for log buffer!");
      return;
    }
  }

  // Initialize write index if needed
  if (xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, false);
    if (!preferences.isKey(NVS_KEY_LOG_INDEX)) {
      preferences.putUInt(NVS_KEY_LOG_INDEX, 0);
    }
    preferences.end();
    xSemaphoreGive(nvsMutex);
  }
}

int getLogEntries(char* buffer, size_t bufferSize) {
  if (buffer == NULL || bufferSize == 0) {
    return 0;
  }

  int entriesRead = 0;
  size_t bufferPos = 0;

  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, true);  // Read-only

    unsigned int writeIndex = preferences.getUInt(NVS_KEY_LOG_INDEX, 0);

    // Read entries in chronological order (oldest to newest)
    for (unsigned int i = 0; i < logBufferMaxEntries; i++) {
      // Calculate read index (start from oldest entry)
      unsigned int readIndex = (writeIndex + i) % logBufferMaxEntries;

      char key[NVS_KEY_MAX_LENGTH];
      snprintf(key, sizeof(key), "%s%02u", NVS_KEY_LOG_PREFIX, readIndex);

      String entry = preferences.getString(key, "");
      if (entry.length() > 0) {
        // Check if entry fits in buffer (including newline and null terminator)
        if (bufferPos + entry.length() + 2 < bufferSize) {
          // Copy entry to buffer
          strcpy(buffer + bufferPos, entry.c_str());
          bufferPos += entry.length();

          // Add newline
          buffer[bufferPos++] = '\n';
          buffer[bufferPos] = '\0';

          entriesRead++;
        } else {
          // Buffer full
          break;
        }
      }
    }

    preferences.end();
    xSemaphoreGive(nvsMutex);
  }

  return entriesRead;
}

void clearLogBuffer() {
  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, false);

    // Clear all log entries (use MAX_LOG_BUFFER_SIZE to clean up old entries if size was reduced)
    for (unsigned int i = 0; i < MAX_LOG_BUFFER_SIZE; i++) {
      char key[NVS_KEY_MAX_LENGTH];
      snprintf(key, sizeof(key), "%s%02u", NVS_KEY_LOG_PREFIX, i);
      preferences.remove(key);
    }

    // Reset write index
    preferences.putUInt(NVS_KEY_LOG_INDEX, 0);

    preferences.end();
    xSemaphoreGive(nvsMutex);
  }
}

size_t getLogBufferSize() {
  size_t totalSize = 0;

  if (nvsMutex != NULL && xSemaphoreTake(nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(NVS_NAMESPACE, true);  // Read-only

    // Count size of all stored log entries
    for (unsigned int i = 0; i < logBufferMaxEntries; i++) {
      char key[NVS_KEY_MAX_LENGTH];
      snprintf(key, sizeof(key), "%s%02u", NVS_KEY_LOG_PREFIX, i);

      String entry = preferences.getString(key, "");
      if (entry.length() > 0) {
        totalSize += entry.length() + 1;  // +1 for newline
      }
    }

    preferences.end();
    xSemaphoreGive(nvsMutex);
  }

  return totalSize;
}
