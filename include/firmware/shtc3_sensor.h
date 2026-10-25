#pragma once

#include <Arduino.h>
#include <Wire.h>

class Shtc3Sensor {
 public:
  explicit Shtc3Sensor(TwoWire& bus);
  void begin(uint32_t now);
  void poll(uint32_t now);
  bool read(float& temperature, float& humidity, uint32_t now) const;

 private:
  enum class State { idle, waking, measuring };
  bool sendCommand(uint16_t command);
  void finishMeasurement(uint32_t now);

  TwoWire& bus_;
  State state_ = State::idle;
  uint32_t lastAttemptAt_ = 0;
  uint32_t stateStartedAt_ = 0;
  uint32_t lastReadingAt_ = 0;
  float temperature_ = NAN;
  float humidity_ = NAN;
  bool valid_ = false;
};
