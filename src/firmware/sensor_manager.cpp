#include "firmware/sensor_manager.h"

#include <SensirionErrors.h>

#include "config.h"
#include "firmware/time_utils.h"

namespace {
constexpr uint8_t PRIMARY_I2C_BUS = 0;
constexpr uint8_t SECONDARY_I2C_BUS = 1;

void printScdError(const char* operation, int16_t error) {
  char message[96];
  errorToString(error, message, sizeof(message));
  Serial.print(operation);
  Serial.print(": ");
  Serial.println(message);
}
}  // namespace

void SensorManager::begin(uint32_t now) {
  sht45_.begin(now);
  shtc3_.begin(now);
  shtReading_.readAt = now - Config::SHT45_READ_INTERVAL_MS;
  startInitialization(PRIMARY_I2C_BUS, now);
}

void SensorManager::poll(uint32_t now) {
  sht45_.poll(now);
  shtc3_.poll(now);
  if (ready_) {
    // A replacement can acknowledge I2C while still being in idle mode.
    if (hasElapsed(now, lastScdMeasurementAt_,
                   Config::SCD_MEASUREMENT_TIMEOUT_MS)) {
      Serial.println("SCD4x measurement timeout; searching again");
      startInitialization(PRIMARY_I2C_BUS, now);
    }
    return;
  }
  if (initializationState_ != InitializationState::idle) {
    continueInitialization(now);
    return;
  }
  if (hasElapsed(now, lastInitializationAt_,
                 Config::SENSOR_RETRY_INTERVAL_MS)) {
    startInitialization(PRIMARY_I2C_BUS, now);
  }
}

void SensorManager::read(Measurement& measurement, uint32_t now) {
  readScd(measurement, now);
  readSht(measurement, now);
  const bool ambientValid = shtc3_.read(measurement.ambientTemperature,
                                       measurement.ambientHumidity, now);
  measurement.ambientTemperatureValid = ambientValid;
  measurement.ambientHumidityValid = ambientValid;
}

void SensorManager::readSht(Measurement& measurement, uint32_t now) {
  if (hasElapsed(now, shtReading_.readAt, Config::SHT45_READ_INTERVAL_MS)) {
    shtReading_.readAt = now;
    shtReading_.valid = sht45_.read(shtReading_.temperature, shtReading_.humidity, now);
    shtReading_.heaterElapsedMs = shtReading_.valid ? sht45_.heaterElapsedMs() : -1;
    // Later valid readings remain cooled, even when acquisition is delayed.
    const bool cooled = shtReading_.valid && shtReading_.heaterElapsedMs >=
        static_cast<int32_t>(Config::SHT45_HEATER_COOLDOWN_MS);
    shtReading_.offset = cooled ? Config::SHT45_HUMIDITY_OFFSET_RH : 0.0f;
  }
  // Keep the last scheduled reading during heating; preserve its original timestamp.
  measurement.shtReadUptimeMs = shtReading_.readAt;
  measurement.boxTemperature = shtReading_.temperature;
  measurement.humidity = shtReading_.humidity;
  measurement.boxTemperatureValid = shtReading_.valid;
  measurement.humidityValid = shtReading_.valid;
  measurement.humidityOffset = shtReading_.offset;
  measurement.shtHeaterElapsedMs = shtReading_.heaterElapsedMs;
}

bool SensorManager::isReady() const {
  return ready_;
}

uint16_t SensorManager::errorCount() const {
  return errorCount_;
}

void SensorManager::startInitialization(uint8_t busIndex, uint32_t now) {
  ready_ = false;
  scdTemperatureOffset_ = NAN;
  scdSerialNumber_ = 0;
  busIndex_ = busIndex;
  TwoWire& bus = busForIndex(busIndex_);
#ifndef ARDUINO_ARCH_ESP32
  bus.begin();
#endif
  bus.setClock(Config::I2C_CLOCK_HZ);
  scd4x_.begin(bus, SCD41_I2C_ADDR_62);
  scd4x_.wakeUp();
  stateStartedAt_ = now;
  initializationState_ = InitializationState::waitingAfterWake;
}

void SensorManager::continueInitialization(uint32_t now) {
  if (initializationState_ == InitializationState::waitingAfterWake &&
      hasElapsed(now, stateStartedAt_, Config::SCD_WAKE_DELAY_MS)) {
    scd4x_.stopPeriodicMeasurement();
    stateStartedAt_ = now;
    initializationState_ = InitializationState::waitingAfterStop;
    return;
  }
  if (initializationState_ == InitializationState::waitingAfterStop &&
      hasElapsed(now, stateStartedAt_, Config::SCD_STOP_DELAY_MS)) {
    finishInitialization(now);
  }
}

void SensorManager::finishInitialization(uint32_t now) {
  uint64_t serialNumber = 0;
  if (scd4x_.getSerialNumber(serialNumber)) {
    tryNextBus(now);
    return;
  }

  int16_t error = scd4x_.setTemperatureOffset(
      Config::SCD41_TEMPERATURE_OFFSET_C);
  if (error) {
    printScdError("offset", error);
    tryNextBus(now);
    return;
  }
  error = scd4x_.getTemperatureOffset(scdTemperatureOffset_);
  if (error) {
    printScdError("read offset", error);
    tryNextBus(now);
    return;
  }
  error = scd4x_.setSensorAltitude(Config::SCD41_ALTITUDE_M);
  if (error) {
    printScdError("altitude", error);
  }
  error = scd4x_.setAutomaticSelfCalibrationEnabled(
      Config::SCD41_ASC_ENABLED);
  if (error) {
    printScdError("ASC", error);
  }
  if (scd4x_.startPeriodicMeasurement()) {
    tryNextBus(now);
    return;
  }

  ready_ = true;
  lastScdMeasurementAt_ = now;
  scdSerialNumber_ = serialNumber;
  errorCount_ = 0;
  initializationState_ = InitializationState::idle;
  Serial.print("SCD4x: ");
  Serial.println(busName(busIndex_));
}

void SensorManager::tryNextBus(uint32_t now) {
#ifndef ARDUINO_ARCH_ESP32
  if (busIndex_ == PRIMARY_I2C_BUS) {
    startInitialization(SECONDARY_I2C_BUS, now);
    return;
  }
#endif
  initializationState_ = InitializationState::idle;
  lastInitializationAt_ = now;
  Serial.println("SCD4x unavailable; retry scheduled");
}

void SensorManager::readScd(Measurement& measurement, uint32_t now) {
  if (!ready_) {
    return;
  }

  bool dataReady = false;
  int16_t error = scd4x_.getDataReadyStatus(dataReady);
  if (!error && dataReady) {
    error = scd4x_.readMeasurement(measurement.co2, measurement.scdTemperature,
                                   measurement.scdHumidity);
  }
  if (error) {
    recordScdError("read", error, now);
    return;
  }
  if (!dataReady || !measurement.co2) {
    return;
  }

  measurement.co2Valid = true;
  lastScdMeasurementAt_ = now;
  measurement.scdSerialNumber = scdSerialNumber_;
  measurement.scdTemperatureOffset = scdTemperatureOffset_;
  measurement.scdHumidityValid = isfinite(measurement.scdHumidity) &&
      measurement.scdHumidity >= Config::HUMIDITY_MIN_RH &&
      measurement.scdHumidity <= Config::HUMIDITY_MAX_RH;
  errorCount_ = 0;
}

void SensorManager::recordScdError(const char* operation, int16_t error,
                                   uint32_t now) {
  printScdError(operation, error);
  ++errorCount_;
  if (errorCount_ < Config::SCD_MAX_CONSECUTIVE_ERRORS) {
    return;
  }
  startInitialization(PRIMARY_I2C_BUS, now);
}

TwoWire& SensorManager::busForIndex(uint8_t busIndex) {
#ifdef ARDUINO_ARCH_ESP32
  return Wire;
#else
  return busIndex == PRIMARY_I2C_BUS ? Wire : Wire1;
#endif
}

const char* SensorManager::busName(uint8_t busIndex) const {
  return busIndex == PRIMARY_I2C_BUS ? "Wire" : "Wire1";
}
