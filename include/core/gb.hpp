#pragma once

#include "bus.hpp"
#include "cart.hpp"
#include "cpu.hpp"
#include "io.hpp"
#include "SDL3/SDL_audio.h"

#include <atomic>
struct GB {
  SM83 cpu;
  Timer timer;
  PPU ppu;
  Bus bus;
  APU apu;
  Cartridge cart;

  GB();
  ~GB();

  void load_cart(const File&);
  void init_hw_regs(SYSTEM_MODE);

  std::atomic<bool> active = true;

  void save_game();
  void load_save_game();
  void system_loop();

  void reset();
};