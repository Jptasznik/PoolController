#if defined(ESP32)

#define LOG_MODULE "OTA"

#include "ESP32OTAHelper.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "Logger.h"
#include "LCDDisplay.h"

// Global variables for progress tracking
static unsigned int g_lastProgress = 0;

// Progress callback function
void otaProgressCallback(unsigned int progress, unsigned int total) {
    // Update LCD every 5% or every 50KB (whichever comes first)
    unsigned int percentNow = (progress * 100) / total;
    unsigned int percentLast = (g_lastProgress * 100) / total;

    if (percentNow != percentLast || (progress - g_lastProgress) >= 51200) {
        displayOTAProgress(progress, total);
        LOG_INFOF("OTA Progress: %u%%  (%u/%u bytes)", percentNow, progress, total);
        g_lastProgress = progress;
    }
}

String startOtaUpdate(const String &url) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    LOG_INFO("Starting OTA update...");
    if (!https.begin(client, url)) {
        LOG_ERROR("Unable to connect to OTA server");
        return "Unable to connect";
    }

    LOG_INFO("Sending GET request...");
    int httpCode = https.GET();
    if (httpCode != HTTP_CODE_OK) {
        LOG_ERRORF("GET failed, error: %s", https.errorToString(httpCode).c_str());
        return "GET... failed, error: " + https.errorToString(httpCode);
    }

    int contentLength = https.getSize();
    LOG_INFOF("OTA firmware size: %d bytes (%d KB)", contentLength, contentLength / 1024);

    if (contentLength == 0) {
        LOG_ERROR("No content length in response");
        return "There was no content length in the response";
    }

    LOG_INFO("Beginning firmware update...");
    bool canBegin = Update.begin(contentLength);

    if (!canBegin) {
        LOG_ERROR("Not enough space to begin OTA");
        return "Not enough space to begin OTA";
    }

    // Reset progress tracking
    g_lastProgress = 0;

    // Setup progress callback
    Update.onProgress(otaProgressCallback);

    WiFiClient *stream  = https.getStreamPtr();
    size_t      written = Update.writeStream(*stream);

    if (written != contentLength) {
        LOG_ERRORF("Written only %u/%u bytes", written, contentLength);
        return "Written only : " + String(written) + "/" + String(contentLength) + ". Retry?";
    }
    LOG_INFOF("Successfully written %u bytes", written);

    if (!Update.end()) {
        LOG_ERRORF("Update end failed. Error #: %u", Update.getError());
        return "Error Occurred. Error #: " + String(Update.getError());
    }
    LOG_INFO("OTA update completed successfully!");

    if (!Update.isFinished()) {
        LOG_ERROR("Update not finished - something went wrong!");
        return "Update not finished? Something went wrong!";
    }

    LOG_INFO("Update successfully completed. Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();

    return "";
}

#endif // ESP32
