#include "firmware/esp32_hardware.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

namespace {
constexpr uint8_t SCD41_ADDRESS = 0x62;

void printDeviceStatus(uint8_t address, const char* name) {
  Wire.beginTransmission(address);
  const uint8_t error = Wire.endTransmission();
  Serial.print(name);
  Serial.print(" I2C address 0x");
  Serial.print(address, HEX);
  Serial.print(" status=");
  Serial.println(error);
}
}  // namespace

void beginEsp32Hardware() {
  Serial.print("PSRAM bytes: ");
  Serial.println(ESP.getPsramSize());
  if (!Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN)) {
    Serial.println("I2C initialization failed");
    return;
  }
  Wire.setClock(Config::I2C_CLOCK_HZ);
  Serial.print("I2C SDA=");
  Serial.print(Config::I2C_SDA_PIN);
  Serial.print(" SCL=");
  Serial.println(Config::I2C_SCL_PIN);
  Serial.print("I2C idle SDA=");
  Serial.print(digitalRead(Config::I2C_SDA_PIN));
  Serial.print(" SCL=");
  Serial.println(digitalRead(Config::I2C_SCL_PIN));
  printDeviceStatus(Config::SHT45_I2C_ADDRESS, "SHT45");
  printDeviceStatus(SCD41_ADDRESS, "SCD41");
}
