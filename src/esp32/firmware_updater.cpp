#include "firmware/firmware_updater.h"

#include <Update.h>

FirmwareUpdater::FirmwareUpdater(QspiStorage&) {}

bool FirmwareUpdater::beginUpload() {
  cancelUpload();
  writtenBytes_ = 0;
  expectedFileSize_ = 0;
  uploadFailed_ = !Update.begin(UPDATE_SIZE_UNKNOWN);
  return !uploadFailed_;
}

bool FirmwareUpdater::write(const uint8_t* data, size_t length) {
  if (uploadFailed_ || !expectedFileSize_) {
    return false;
  }
  const size_t count = min(length, expectedFileSize_ - writtenBytes_);
  if (count && Update.write(const_cast<uint8_t*>(data), count) != count) {
    uploadFailed_ = true;
    return false;
  }
  writtenBytes_ += count;
  return true;
}

FirmwareUpdateResult FirmwareUpdater::finishAndInstall() {
  if (uploadFailed_ || !expectedFileSize_ || writtenBytes_ != expectedFileSize_) {
    cancelUpload();
    return {400, "Incomplete or invalid firmware upload.", false};
  }
  if (!Update.end(true)) {
    return {422, "Firmware verification failed: " + String(Update.errorString()), false};
  }
  return {200, "Firmware verified; restarting.", true};
}

void FirmwareUpdater::cancelUpload() {
  if (Update.isRunning()) {
    Update.abort();
  }
}

void FirmwareUpdater::restart() {
  delay(500);
  ESP.restart();
}
