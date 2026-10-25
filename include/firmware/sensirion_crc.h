#pragma once

#include <stdint.h>

inline bool sensirionChecksumValid(const uint8_t* word) {
  constexpr uint8_t CRC_POLYNOMIAL = 0x31;
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < 2; ++i) {
    crc ^= word[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? (crc << 1) ^ CRC_POLYNOMIAL : crc << 1;
    }
  }
  return crc == word[2];
}
