#include <array>

#include "common.h"

const std::array<u32, 4> CLOCK_SELECT_TABLE = {1024, 16, 64, 256};
const std::array<u32, 4> TIMER_BIT          = {9, 3, 5, 7};

struct Timer {
  bool overflow_update_queued = false;
  u8 prev_and_result          = 0;

  // DIV
  u16 div = 0xAB << 8;

  // TAC
  bool ticking_enabled    = false;
  u32 increment_frequency = 0;

  bool prev_behaviour = false;
  // TIMA
  u8 counter = 0x0;

  // TMA
  u8 modulo = 0x0;

  u8 get_div() { return div >> 8; }
  void set_tac(u8 value) {
    ticking_enabled = (value & 0x4) ? true : false;
    // fmt::println("ticking enabled: {}", ticking_enabled);
    increment_frequency = CLOCK_SELECT_TABLE[value & 0x3];
  }
};