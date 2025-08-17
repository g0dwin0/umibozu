#pragma once
#include "bus.hpp"
#include "common.hpp"

static constexpr u32 CLOCK_SPEED = 4194304;

enum class WRITING_MODE { RTC, RAM };

class Mapper {
 public:
  static inline Bus* bus     = nullptr;
  u8 id                      = 0x00;
  u8 banking_mode            = 0;
  u16 rom_bank               = 0;
  u16 ram_bank               = 0;
  WRITING_MODE register_mode = WRITING_MODE::RAM;

  // RTC
  u64 rtc_internal_clock = 0;  // RTC only

  enum class RTC_REGISTER { RTC_SECOND_TIME = 0x08, RTC_MINUTE_TIME = 0x09, RTC_HOUR_TIME = 0x0A, RTC_DAY_LOW = 0x0B, RTC_DAY_HIGH = 0x0C };

  struct RTC_INSTANCE {
    u8 RTC_SECOND_TIME : 6 = 0;
    u8 RTC_MINUTE_TIME : 6 = 0;
    u8 RTC_HOUR_TIME   : 5 = 0;
    u16 RTC_DAY            = 0;

    void write_to_active_reg(RTC_REGISTER r, u8 v, u64& internal_clock) {
      switch (r) {
        case RTC_REGISTER::RTC_SECOND_TIME: {
          RTC_SECOND_TIME = v & 0b00111111;
          internal_clock  = 0;
          break;
        }
        case RTC_REGISTER::RTC_MINUTE_TIME: {
          RTC_MINUTE_TIME = v & 0b00111111;
          break;
        }
        case RTC_REGISTER::RTC_HOUR_TIME: {
          RTC_HOUR_TIME = v & 0b00011111;
          break;
        }
        case RTC_REGISTER::RTC_DAY_LOW: {
          RTC_DAY &= 0b1111111100000000;
          RTC_DAY |= (v & 0b11111111);
          break;
        }
        case RTC_REGISTER::RTC_DAY_HIGH: {
          RTC_DAY &= 0b0000000011111111;
          RTC_DAY |= ((v & 0b11000001) << 8);
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
        case RTC_REGISTER::RTC_SECOND_TIME: {
          return RTC_SECOND_TIME;
        }
        case RTC_REGISTER::RTC_MINUTE_TIME: {
          return RTC_MINUTE_TIME;
        }
        case RTC_REGISTER::RTC_HOUR_TIME: {
          return RTC_HOUR_TIME;
        }
        case RTC_REGISTER::RTC_DAY_LOW: {
          return (RTC_DAY & 0xFF);
        }
        case RTC_REGISTER::RTC_DAY_HIGH: {
          return (RTC_DAY >> 8);
        }
        default: {
          fmt::println("[MAPPER] invalid rtc mode ({})", (u8)r);
          exit(-1);
        }
      }
    }
  };

  void tick_rtc() {
    // fmt::println("[RTC] RTC_SECOND_TIME: {}", (u8)actual.RTC_SECOND_TIME);
    actual.RTC_SECOND_TIME += 1;

    if (actual.RTC_SECOND_TIME == 60) {
      actual.RTC_SECOND_TIME = 0;
      actual.RTC_MINUTE_TIME += 1;
      if (actual.RTC_MINUTE_TIME == 60) {
        actual.RTC_MINUTE_TIME = 0;
        actual.RTC_HOUR_TIME += 1;

        if (actual.RTC_HOUR_TIME == 24) {
          actual.RTC_HOUR_TIME = 0;
          if (actual.RTC_DAY == 0x1ff) {
            actual.RTC_DAY &= ~0x1ff;
            actual.RTC_DAY |= (1 << 15);
          } else {
            actual.RTC_DAY += 1;
          }
        }
      }
    }
  }

  void increment_internal_clock(u8 amount, const u16& RTC_DAY) {
    if (!rtc_ext_ram_enabled) return;
    if ((RTC_DAY & (1 << 14)) != 0) return;

    rtc_internal_clock += amount;

    if (rtc_internal_clock == CLOCK_SPEED) {
      tick_rtc();
      rtc_internal_clock = 0;
    }
  }

  RTC_REGISTER active_rtc_register = RTC_REGISTER::RTC_SECOND_TIME;
  u8 last_rtc_value                = 0xFF;
  bool latched_occured             = false;
  RTC_INSTANCE latched, actual = {};
  bool rtc_ext_ram_enabled = false;

  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;

};

Mapper* get_mapper_by_id(u8 mapper_id);