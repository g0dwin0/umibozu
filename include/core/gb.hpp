#pragma once

#include "bus.hpp"
#include "cpu.hpp"
#include "io.hpp"
#include "apu.hpp"

struct GB {
  SM83 cpu = {};
  Timer timer = {}; // DIV etc....
  PPU ppu = {};
  Bus bus = {};
  APU apu = {};

  GB();
  // ~GB();

  void load_cart(const File&);
  void init_hw_regs(SYSTEM_MODE);
  
  void load_save_game();
};