#pragma once

#include "bus.h"
#include "cpu.h"
#include "io.hpp"
#include "apu.h"

struct GB {
  SM83 cpu;
  Timer timer; // DIV etc....
  PPU ppu;
  Bus bus;
  APU apu;

  GB();
  ~GB();

  void load_cart(const File&);
  void init_hw_regs(SYSTEM_MODE);
  
  void load_save_game();
};