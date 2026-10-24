#include "firmware/qspi_storage.h"

#include <LittleFS.h>
#include <esp_system.h>

namespace {
constexpr uint32_t BOOT_ID_FALLBACK_MASK = 0xA5C3417D;
constexpr char BOOT_ID_PATH[] = "/littlefs/BOOT.ID";
}  // namespace

void QspiStorage::begin() {
  dataReady_ = LittleFS.begin(false);
  if (!dataReady_) {
    // Provision the data partition on first installation; OTA leaves it intact.
    dataReady_ = LittleFS.format() && LittleFS.begin(false);
  }
  otaReady_ = true;
  Serial.println(dataReady_ ? "LittleFS ready" : "LittleFS unavailable");
}

uint32_t QspiStorage::nextBootId() {
  uint32_t previousBootId = 0;
  if (dataReady_) {
    FILE* file = fopen(BOOT_ID_PATH, "rb");
    if (file) {
      if (fread(&previousBootId, sizeof(previousBootId), 1, file) != 1) {
        previousBootId = 0;
      }
      fclose(file);
    }
  }

  uint32_t bootId = esp_random();
  if (!bootId || bootId == previousBootId) {
    bootId = previousBootId + 1;
    if (!bootId) {
      bootId = BOOT_ID_FALLBACK_MASK;
    }
  }

  if (dataReady_) {
    FILE* file = fopen(BOOT_ID_PATH, "wb");
    if (file) {
      if (fwrite(&bootId, sizeof(bootId), 1, file) != 1) {
        Serial.println("Could not persist boot ID");
      }
      fflush(file);
      fclose(file);
    } else {
      Serial.println("Could not open boot ID file");
    }
  }
  return bootId;
}
