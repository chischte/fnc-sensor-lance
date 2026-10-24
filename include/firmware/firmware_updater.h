#pragma once

#include <Arduino.h>
#ifndef ARDUINO_ARCH_ESP32
#include <Arduino_Portenta_OTA.h>
#include <cstdio>
#endif

#include "firmware/qspi_storage.h"

struct FirmwareUpdateResult {
  uint16_t httpStatus;
  String message;
  bool restartRequired;
};

class FirmwareUpdater {
 public:
  explicit FirmwareUpdater(QspiStorage& storage);

  bool beginUpload();
#ifdef ARDUINO_ARCH_ESP32
  void setExpectedSize(size_t size) { expectedFileSize_ = size; }
#endif
  bool write(const uint8_t* data, size_t length);
  FirmwareUpdateResult finishAndInstall();
  void cancelUpload();
  void restart();

 private:
#ifndef ARDUINO_ARCH_ESP32
  void readFileSize(const uint8_t* data, size_t length);
  size_t writableBytes(size_t requestedBytes) const;
  void closeUpload();

  QspiStorage& storage_;
  Arduino_Portenta_OTA_QSPI ota_;
  FILE* uploadFile_ = nullptr;
  uint8_t filePrefix_[4] = {};
  uint8_t prefixBytes_ = 0;
  bool fileSizeKnown_ = false;
#endif
  size_t expectedFileSize_ = 0;
  size_t writtenBytes_ = 0;
  bool uploadFailed_ = false;
};
