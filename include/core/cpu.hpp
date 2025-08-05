#pragma once
#include <spdlog/logger.h>

#include <array>
#include <string>
#include <unordered_map>

#include "bus.hpp"
#include "common.hpp"
#include "enums.hpp"
#include "mapper.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"

static constexpr u32 VBLANK_INTERRUPT = 0x40;
static constexpr u32 STAT_INTERRUPT   = 0x48;
static constexpr u32 TIMER_INTERRUPT  = 0x50;
static constexpr u32 SERIAL_INTERRUPT = 0x58;
static constexpr u32 JOYPAD_INTERRUPT = 0x60;

struct SM83 {
  Bus *bus = nullptr;
#ifndef CPU_TEST_MODE
  std::shared_ptr<spdlog::logger> cpu_logger = spdlog::stdout_color_mt("SM83");
#endif
  std::unordered_map<STATUS, std::string> cpu_mode = {
      {   STATUS::ACTIVE, "ACTIVE"},
      {STATUS::HALT_MODE,   "HALT"},
      {     STATUS::STOP,   "STOP"},
      {   STATUS::PAUSED, "PAUSED"},
  };

  const std::array<u8, 5> INTERRUPT_TABLE = {VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT, JOYPAD_INTERRUPT};

  union {
    u16 AF;
    struct {
      u8 F;
      u8 A;
    };
  };
  union {
    u16 BC;
    struct {
      u8 C;
      u8 B;
    };
  };
  union {
    u16 DE;
    struct {
      u8 E;
      u8 D;
    };
  };
  union {
    u16 HL;
    struct {
      u8 L;
      u8 H;
    };
  };

  u16 SP        = 0xFFFE;
  u16 PC        = 0x0100;
  STATUS status = STATUS::PAUSED;
  bool IME      = false;
  SPEED speed   = SPEED::NORMAL;

  // State
  [[nodiscard]] std::string get_cpu_mode_string() const { return cpu_mode.at(status); };

  // CPU Internals
  void step();
  void handle_interrupts();

  void push_to_stack(const u8 value) {
    // fmt::println("PUSHING TO STACK: {:#010x} -> {:#010X}", SP - 1, value);
    bus->write8(--SP, value);
  }
  u8 pull_from_stack() { return bus->read8(SP++); }

  // Flags
  void set_flag(const FLAG);
  void unset_flag(const FLAG);
  [[nodiscard]] u8 get_flag(const FLAG) const;
  void set_zero() { set_flag(FLAG::ZERO); }
  void set_negative() { set_flag(FLAG::NEGATIVE); }
  void set_half_carry() { set_flag(FLAG::HALF_CARRY); }
  void set_carry() { set_flag(FLAG::CARRY); }
  void reset_zero() { unset_flag(FLAG::ZERO); }
  void reset_negative() { unset_flag(FLAG::NEGATIVE); }
  void reset_half_carry() { unset_flag(FLAG::HALF_CARRY); }
  void reset_carry() { unset_flag(FLAG::CARRY); }

  // HDMA (GBC)
  // void init_hdma(u16 length);
  // void terminate_hdma();
};