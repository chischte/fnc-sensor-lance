#include "firmware/shtc3_sensor.h"

#include "config.h"
#include "firmware/sensirion_crc.h"
#include "firmware/time_utils.h"

namespace {
// Normal-power, temperature-first measurement without clock stretching.
constexpr uint16_t WAKE_UP = 0x3517;
constexpr uint16_t MEASURE = 0x7866;
constexpr uint16_t SLEEP = 0xB098;
constexpr uint32_t WAKE_TIME_MS = 1;
constexpr uint32_t CONVERSION_TIME_MS = 13;
constexpr uint8_t RESPONSE_BYTES = 6;
constexpr uint32_t MAX_READING_AGE_MS = 2 * Config::MEASUREMENT_INTERVAL_MS;
}  // namespace

Shtc3Sensor::Shtc3Sensor(TwoWire& bus) : bus_(bus) {}

void Shtc3Sensor::begin(uint32_t now) {
  state_ = State::idle;
  valid_ = false;
  lastAttemptAt_ = now - Config::MEASUREMENT_INTERVAL_MS;
  poll(now);
}

bool Shtc3Sensor::sendCommand(uint16_t command) {
  bus_.beginTransmission(Config::SHTC3_I2C_ADDRESS);
  bus_.write(static_cast<uint8_t>(command >> 8));
  bus_.write(static_cast<uint8_t>(command));
  return bus_.endTransmission() == 0;
}

void Shtc3Sensor::poll(uint32_t now) {
  if (state_ == State::waking) {
    if (!hasElapsed(now, stateStartedAt_, WAKE_TIME_MS)) return;
    if (!sendCommand(MEASURE)) {
      valid_ = false;
      state_ = State::idle;
      return;
    }
    state_ = State::measuring;
    stateStartedAt_ = now;
    return;
  }
  if (state_ == State::measuring) {
    if (hasElapsed(now, stateStartedAt_, CONVERSION_TIME_MS)) {
      finishMeasurement(now);
    }
    return;
  }
  if (!hasElapsed(now, lastAttemptAt_, Config::MEASUREMENT_INTERVAL_MS)) return;
  lastAttemptAt_ = now;
  if (!sendCommand(WAKE_UP)) {
    valid_ = false;
    return;
  }
  state_ = State::waking;
  stateStartedAt_ = now;
}

void Shtc3Sensor::finishMeasurement(uint32_t now) {
  state_ = State::idle;
  valid_ = false;
  uint8_t data[RESPONSE_BYTES];
  const bool complete = bus_.requestFrom(Config::SHTC3_I2C_ADDRESS,
                                          RESPONSE_BYTES) == RESPONSE_BYTES;
  if (complete) {
    for (uint8_t i = 0; i < RESPONSE_BYTES; ++i) data[i] = bus_.read();
  }
  const bool sleeping = sendCommand(SLEEP);
  if (!complete || !sleeping || !sensirionChecksumValid(data) ||
      !sensirionChecksumValid(data + 3)) return;

  const uint16_t temperatureTicks = (data[0] << 8) | data[1];
  const uint16_t humidityTicks = (data[3] << 8) | data[4];
  temperature_ = -45.0f + 175.0f * temperatureTicks / 65536.0f;
  humidity_ = 100.0f * humidityTicks / 65536.0f;
  lastReadingAt_ = now;
  valid_ = true;
}

bool Shtc3Sensor::read(float& temperature, float& humidity, uint32_t now) const {
  const bool fresh = valid_ && !hasElapsed(now, lastReadingAt_, MAX_READING_AGE_MS);
  temperature = fresh ? temperature_ : NAN;
  humidity = fresh ? humidity_ : NAN;
  return fresh;
}
