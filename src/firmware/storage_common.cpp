#include "firmware/qspi_storage.h"

#include <sys/stat.h>

#include "config.h"
#include "firmware/measurement_json.h"

namespace {
#ifdef ARDUINO_ARCH_ESP32
constexpr char LOG_PATH[] = "/littlefs/MEASUREMENTS.NDJSON";
constexpr char OLD_LOG_PATH[] = "/littlefs/MEASUREMENTS.OLD";
#else
constexpr char LOG_PATH[] = "/data/MEASUREMENTS.NDJSON";
constexpr char OLD_LOG_PATH[] = "/data/MEASUREMENTS.OLD";
#endif
constexpr const char* BACKLOG_PATHS[] = {OLD_LOG_PATH, LOG_PATH};
constexpr size_t BACKLOG_FILE_COUNT = sizeof(BACKLOG_PATHS) / sizeof(BACKLOG_PATHS[0]);
}  // namespace

bool BacklogReader::begin(bool storageReady, bool currentOnly) {
  close();
  nextFileIndex_ = currentOnly ? BACKLOG_FILE_COUNT - 1 : 0;
  finished_ = !storageReady;
  return storageReady && openNextFile();
}

size_t BacklogReader::read(uint8_t* buffer, size_t capacity) {
  while (file_) {
    const size_t bytesRead = fread(buffer, 1, capacity, file_);
    if (bytesRead) {
      return bytesRead;
    }
    fclose(file_);
    file_ = nullptr;
    openNextFile();
  }
  return 0;
}

bool BacklogReader::isFinished() const {
  return finished_;
}

void BacklogReader::close() {
  if (file_) {
    fclose(file_);
    file_ = nullptr;
  }
  finished_ = true;
}

bool BacklogReader::openNextFile() {
  while (nextFileIndex_ < BACKLOG_FILE_COUNT) {
    file_ = fopen(BACKLOG_PATHS[nextFileIndex_], "r");
    ++nextFileIndex_;
    if (file_) {
      finished_ = false;
      return true;
    }
  }
  finished_ = true;
  return false;
}

void QspiStorage::append(const Measurement& measurement, uint32_t bootId) {
  if (!dataReady_) {
    return;
  }

  rotateLogIfNeeded();
  if (!dataReady_) {
    return;
  }
  FILE* file = fopen(LOG_PATH, "a");
  if (!file) {
    dataReady_ = false;
    return;
  }

  String line;
  line.reserve(220);
  appendMeasurementJson(line, measurement, bootId);
  line += '\n';
  if (fwrite(line.c_str(), 1, line.length(), file) != line.length()) {
    dataReady_ = false;
  }
  fflush(file);
  fclose(file);
}

bool QspiStorage::isDataReady() const {
  return dataReady_;
}

bool QspiStorage::isOtaReady() const {
  return otaReady_;
}

void QspiStorage::rotateLogIfNeeded() {
  struct stat status {};
  if (stat(LOG_PATH, &status) ||
      static_cast<size_t>(status.st_size) <
          Config::PERSISTENT_LOG_MAX_BYTES) {
    return;
  }
  remove(OLD_LOG_PATH);
  if (rename(LOG_PATH, OLD_LOG_PATH)) {
    dataReady_ = false;
    Serial.println("Could not rotate measurement log");
  }
}
