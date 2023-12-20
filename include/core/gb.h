#pragma once
#include <limits>

#include "bus.h"
#include "common.h"
#include "cpu.h"
#include "io.hpp"

struct GB {
  SharpSM83 cpu;
  PPU ppu;
  Bus bus;
  GB();

  void load_cart(const File&);
  void init_hw_regs(COMPAT_MODE);
};