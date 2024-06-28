#include <array>

#include "common.h"

const std::array<u8, 4> TIMER_BIT          = {9, 3, 5, 7};

struct Timer {
  bool overflow_update_queued = false;
  u8 prev_and_result          = 0;

  // DIV
  u16 div = 0xAB << 8;

  // TAC
  bool ticking_enabled    = false;

  // TIMA
  u8 counter = 0x0;

  // TMA
  u8 modulo = 0x0;

  void set_tac(u8 value) {
    ticking_enabled = (value & 0x4) != 0;
  }
  
  [[nodiscard]] u8 get_div() const { return div >> 8; }
};