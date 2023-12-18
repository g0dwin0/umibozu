#pragma once
#include "bus.h"
#include "common.h"
#pragma GCC diagnostic ignored "-Wtype-limits"
// TODO: implement MBC1M (>1mb carts)
#define SERIAL_PORT_BUFFER_SIZE 2048

enum RTC_REGISTER {
  RTC_SECOND_TIME = 0x08,
  RTC_MINUTE_TIME = 0x09,
  RTC_HOUR_TIME   = 0x0A,
  RTC_DAY_LOW     = 0x0B,
  RTC_DAY_HIGH    = 0x0C
};

class Mapper {
 public:
  static inline Bus* bus = nullptr;
  u8 banking_mode        = 0;
  u32 rom_bank           = 0;
  u32 ram_bank           = 0;

  bool ram_rtc_enabled                                   = false;
  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;

  u8 handle_system_memory_read(const u16 address);
  void handle_system_memory_write(const u16 address, const u8 value);
};

Mapper* get_mapper_by_id(u8 mapper_id);