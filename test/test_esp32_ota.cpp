#include <Update.h>
#include "firmware/firmware_updater.h"
#define assert(condition) do { if (!(condition)) return __LINE__; } while (false)
int main() {
  QspiStorage storage;
  FirmwareUpdater updater(storage);
  const uint8_t payload[] = {0xE9, 1, 2, 3, 4, 5, 6, 7};

  // Multipart trailers must never reach the flash writer, even across chunks.
  assert(updater.beginUpload());
  updater.setExpectedSize(5);
  assert(updater.write(payload, 3));
  assert(updater.write(payload + 3, 5));
  assert(updater.write(payload, 2));
  assert(Update.bytes == 5);
  auto result = updater.finishAndInstall();
  assert(result.httpStatus == 200 && result.restartRequired && Update.installed);

  assert(updater.beginUpload());
  updater.setExpectedSize(8);
  assert(updater.write(payload, 3));
  result = updater.finishAndInstall();
  assert(result.httpStatus == 400 && !result.restartRequired && !Update.installed);
  assert(!Update.running);

  assert(updater.beginUpload());
  updater.setExpectedSize(8);
  Update.rejectWrite = true;
  assert(!updater.write(payload, 8));
  result = updater.finishAndInstall();
  assert(!result.restartRequired && !Update.running);
  Update.rejectWrite = false;

  assert(updater.beginUpload());
  updater.setExpectedSize(8);
  assert(updater.write(payload, 8));
  Update.rejectImage = true;
  result = updater.finishAndInstall();
  assert(result.httpStatus == 422 && !result.restartRequired && !Update.installed);
  Update.rejectImage = false;

  assert(updater.beginUpload());
  updater.cancelUpload();
  assert(!Update.running && !ESP.restarted);
  return 0;
}
