#pragma once
#include <stddef.h>
#include <stdint.h>
struct String {
  String(const char*) {}
};
inline String operator+(const char*, const String&) { return String(""); }
template <typename T> T min(T left, T right) { return left < right ? left : right; }
inline void delay(unsigned long) {}
struct FakeEsp { bool restarted = false; void restart() { restarted = true; } };
inline FakeEsp ESP;
