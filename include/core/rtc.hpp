#pragma once
#include "common.hpp"
enum class RTC_REGISTER { RTC_SECOND_TIME = 0x08, RTC_MINUTE_TIME = 0x09, RTC_HOUR_TIME = 0x0A, RTC_DAY_LOW = 0x0B, RTC_DAY_HIGH = 0x0C };

struct RTC_INSTANCE {
  u64 internal_clock = 0;  // RTC only
  RTC_REGISTER active_register = RTC_REGISTER::RTC_SECOND_TIME;


  u8 second_time : 6 = 0;
  u8 minute_time : 6 = 0;
  u8 hour_time   : 5 = 0;
  u16 day            = 0;

  void write_to_active_reg(RTC_REGISTER r, u8 v, u64& internal_clock) {
    switch (r) {
      case RTC_REGISTER::RTC_SECOND_TIME: {
        second_time    = v & 0b00111111;
        internal_clock = 0;
        break;
      }
      case RTC_REGISTER::RTC_MINUTE_TIME: {
        minute_time = v & 0b00111111;
        break;
      }
      case RTC_REGISTER::RTC_HOUR_TIME: {
        hour_time = v & 0b00011111;
        break;
      }
      case RTC_REGISTER::RTC_DAY_LOW: {
        day &= 0b1111111100000000;
        day |= (v & 0b11111111);
        break;
      }
      case RTC_REGISTER::RTC_DAY_HIGH: {
        day &= 0b0000000011111111;
        day |= ((v & 0b11000001) << 8);
        break;
      }
      default: {
        fmt::println("[MAPPER] invalid rtc mode ({})", (u8)r);
        exit(-1);
      }
    }
  }

  [[nodiscard]] u16 read_from_active_reg(RTC_REGISTER r) {
    switch (r) {
      case RTC_REGISTER::RTC_SECOND_TIME: return second_time;
      case RTC_REGISTER::RTC_MINUTE_TIME: return minute_time;
      case RTC_REGISTER::RTC_HOUR_TIME: return hour_time;
      case RTC_REGISTER::RTC_DAY_LOW: return (day & 0xFF);
      case RTC_REGISTER::RTC_DAY_HIGH: return (day >> 8);
      default: assert(0);
    }
  }

  void tick() {
    // fmt::println("[RTC] RTC_SECOND_TIME: {}", (u8)RTC_SECOND_TIME);
    second_time += 1;

    if (second_time == 60) {
      second_time = 0;
      minute_time += 1;
      if (minute_time == 60) {
        minute_time = 0;
        hour_time += 1;

        if (hour_time == 24) {
          hour_time = 0;
          if (day == 0x1ff) {
            day &= ~0x1ff;
            day |= (1 << 15);
          } else {
            day += 1;
          }
        }
      }
    }
  }
};