#pragma once

#include <Arduino.h>

struct Measurement {
  uint32_t sequence = 0;
  uint32_t uptimeMs = 0;
  uint16_t co2 = 0;
  float boxTemperature = NAN;
  uint32_t shtReadUptimeMs = 0;
  float humidity = NAN;
  float humidityOffset = 0.0f;
  int32_t shtHeaterElapsedMs = -1;
  bool shtHeated = false;
  bool shtCooling = false;
  float scdHumidity = NAN;
  bool scdHumidityValid = false;
  float scdTemperature = NAN;
  float scdTemperatureOffset = NAN;
  uint64_t scdSerialNumber = 0;
  bool co2Valid = false;
  bool boxTemperatureValid = false;
  bool humidityValid = false;
};
