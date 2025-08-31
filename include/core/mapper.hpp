#pragma once
#include "bus.hpp"
#include "common.hpp"
#include "rtc.hpp"

static constexpr u32 CLOCK_SPEED = 4194304;

enum class WRITING_MODE { RTC, RAM };

class Mapper {
  public:
  virtual ~Mapper() {};
  static inline Bus* bus     = nullptr;
  u8 id                      = 0x00;
  u8 banking_mode            = 0;
  u16 rom_bank               = 0;
  u16 ram_bank               = 0;
  WRITING_MODE register_mode = WRITING_MODE::RAM;

  // RTC & RAM banks share the same enable bit
  bool rtc_ext_ram_enabled = false;

  // RTC -- refactor: is better if this is abstracted away, too much cluster.
  RTC_INSTANCE rtc_latched, rtc_actual = {};

  void increment_internal_clock(u8 amount, const u16& RTC_DAY) {
    if (!rtc_ext_ram_enabled) return;
    if ((RTC_DAY & (1 << 14)) != 0) return;

    rtc_actual.internal_clock += amount;

    if (rtc_actual.internal_clock == CLOCK_SPEED) {
      rtc_actual.tick();
      rtc_actual.internal_clock = 0;
    }
  }

  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;
};

Mapper* get_mapper_by_id(u8 mapper_id);