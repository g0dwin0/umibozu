#pragma once
#include "bus.h"
#include "common.h"
#pragma GCC diagnostic ignored "-Wtype-limits"
#define SERIAL_PORT_BUFFER_SIZE 2048

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

  enum class RTC_REGISTERS : u8 {
    RTC_SECOND_TIME = 0x08,
    RTC_MINUTE_TIME = 0x09,
    RTC_HOUR_TIME   = 0x0A,
    RTC_DAY_LOW     = 0x0B,
    RTC_DAY_HIGH    = 0x0C
  };
  struct RTC_INSTANCE {
    u8 RTC_SECOND_TIME : 6 = 0;
    u8 RTC_MINUTE_TIME : 6 = 0;
    u8 RTC_HOUR_TIME   : 5 = 0;
    u16 RTC_DAY            = 0;

    // void print_rtc_regs() {
    //   fmt::println("[RTC] SECONDS: ")
    // }
    void write_to_active_reg(RTC_REGISTERS r, u8 v, u64& internal_clock) {
      switch (r) {
        case RTC_REGISTERS::RTC_SECOND_TIME: {
          RTC_SECOND_TIME = v & 0b00111111;
          // fmt::println("wrote {:#08x} to RTC_SECOND_TIME {:#08x}", v,
          // (u8)RTC_SECOND_TIME);
          internal_clock = 0;
          break;
        }
        case RTC_REGISTERS::RTC_MINUTE_TIME: {
          RTC_MINUTE_TIME = v & 0b00111111;
          // fmt::println("wrote {:#08x} to RTC_SECOND_TIME {:#08x}", v,
          // (u8)RTC_SECOND_TIME);
          break;
        }
        case RTC_REGISTERS::RTC_HOUR_TIME: {
          RTC_HOUR_TIME = v & 0b00011111;
          break;
        }
        case RTC_REGISTERS::RTC_DAY_LOW: {
          // fmt::println("LOW OLD: {:08b}", RTC_DAY & 0xFF);
          // fmt::println("WRITING VAL TO LOW: {:08b}", v);
          RTC_DAY &= 0b1111111100000000;
          RTC_DAY |= (v & 0b11111111);
          // fmt::println("LOW NEW: {:08b}", RTC_DAY & 0xFF);
          break;
        }
        case RTC_REGISTERS::RTC_DAY_HIGH: {
          // fmt::println("high OLD: {:08b}", RTC_DAY >> 8);
          // fmt::println("RTC DAY FULL: {:016b}", RTC_DAY);
          // print_rtc_regs();
          // fmt::println("WRITING VAL TO HIGH: {:08b}", v);
          RTC_DAY &= 0b0000000011111111;
          
          // if((v & (1 << 6)) != 0) internal_clock -= 2;

          RTC_DAY |= ((v & 0b11000001) << 8);
          // fmt::println("RTC DAY FULL: {:016b}", RTC_DAY);
          // fmt::println("high NEW: {:08b}", RTC_DAY >> 8);
          break;
        }
        default: {
          fmt::println("[MAPPER] invalid rtc mode ({})", (u8)r);
          exit(-1);
        }
      }
    }

    u16 read_from_active_reg(RTC_REGISTERS r) {
      switch (r) {
        case RTC_REGISTERS::RTC_SECOND_TIME: {
          return RTC_SECOND_TIME;
        }
        case RTC_REGISTERS::RTC_MINUTE_TIME: {
          return RTC_MINUTE_TIME;
          break;
        }
        case RTC_REGISTERS::RTC_HOUR_TIME: {
          return RTC_HOUR_TIME;
          break;
        }
        case RTC_REGISTERS::RTC_DAY_LOW: {
          return (RTC_DAY & 0xFF);
          break;
        }
        case RTC_REGISTERS::RTC_DAY_HIGH: {
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
      // fmt::println("[RTC] added minute to RTC_MINUTE_TIME");
      actual.RTC_SECOND_TIME = 0;
      actual.RTC_MINUTE_TIME += 1;
      // fmt::println("[RTC] RTC_MINUTE_TIME: {}", (u8)actual.RTC_MINUTE_TIME);
      if (actual.RTC_MINUTE_TIME == 60) {
        // fmt::println("[RTC] RTC_HOUR_TIME: {}", (u8)actual.RTC_HOUR_TIME);
        actual.RTC_MINUTE_TIME = 0;
        actual.RTC_HOUR_TIME += 1;

        if (actual.RTC_HOUR_TIME == 24) {
          // fmt::println("[RTC] RTC_DAY_TIME: {}", (u16)actual.RTC_DAY >> 7);
          actual.RTC_HOUR_TIME = 0;
          if (actual.RTC_DAY == 0b111111111) {
            actual.RTC_DAY &= ~0b111111111;
            actual.RTC_DAY |= 1 << 15;
          } else {
            actual.RTC_DAY += 1;
          }
        }
      }
    }
  }

  void increment_internal_clock(u8 amount, const u16& RTC_DAY) {
    if (!rtc_enabled) return;
    if ((RTC_DAY & (1 << 14)) != 0) {
      // fmt::println("RTC day: {:016b}", RTC_DAY);
      return;
    }

    rtc_internal_clock += amount;

    // fmt::println("[MBC3] internal clock: {}", internal_clock);
    if (rtc_internal_clock == 4194304) {
      tick_rtc();
      rtc_internal_clock = 0;
    }
  }

  RTC_REGISTERS active_rtc_register = RTC_REGISTERS::RTC_SECOND_TIME;
  u8 last_rtc_value                 = 0xFF;
  bool latched_occured              = false;
  RTC_INSTANCE latched, actual;
  bool ext_ram_enabled = false;
  bool rtc_enabled     = false;

  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;

  u8 handle_system_memory_read(const u16 address);
  void handle_system_memory_write(const u16 address, const u8 value);
};

Mapper* get_mapper_by_id(u8 mapper_id);