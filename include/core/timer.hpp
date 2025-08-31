#pragma once
#include <array>

#include "common.hpp"
struct Bus;
#include "bus.hpp"

const std::array<u8, 4> TIMER_BIT = {9, 3, 5, 7};

struct Timer {
  bool overflow_update_queued = false;
  u8 prev_and_result          = 0;

  // TAC
  bool ticking_enabled = false;

  // TIMA
  u8 counter = 0x0;

  // TMA
  u8 modulo = 0x0;

  void set_tac(const u8 value);
  void increment_div(const u8 value, bool);
  void reset_div(bool);

  [[nodiscard]] u8 get_div() const { return div >> 8; }

  [[nodiscard]] u16 get_full_div() const { return div; }

  Bus* bus = nullptr;
 private:
  u16 div  = 0xAB << 8;
};