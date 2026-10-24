#pragma once
#include <Arduino.h>
constexpr size_t UPDATE_SIZE_UNKNOWN = 0xFFFFFFFF;
struct FakeUpdate {
  bool running = false;
  bool rejectWrite = false;
  bool rejectImage = false;
  bool installed = false;
  size_t bytes = 0;
  bool begin(size_t) { running = true; bytes = 0; installed = false; return true; }
  size_t write(uint8_t*, size_t count) {
    if (rejectWrite) return 0;
    bytes += count;
    return count;
  }
  bool end(bool) { running = false; installed = !rejectImage; return installed; }
  bool isRunning() { return running; }
  void abort() { running = false; }
  const char* errorString() { return "Invalid image"; }
};
inline FakeUpdate Update;
