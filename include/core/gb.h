#pragma once

#include "bus.h"
#include "cpu.h"
#include "io.hpp"

struct GB {
  SM83 cpu;
  PPU ppu;
  Bus bus;
  GB();
  ~GB();

  void load_cart(const File&);
  void init_hw_regs(COMPAT_MODE);
  void load_save_game();
};