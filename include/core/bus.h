#pragma once
#include "cart.h"
#include "common.h"
#include "joypad.h"

using namespace Umibozu;

enum class InterruptType : u8 {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};

enum class COMPAT_MODE : u8 {
  DMG,
  CGB_ONLY = 0xC0
};

struct PaletteSpecification {
  u8 address : 6      = 0x0;
  bool auto_increment = false;
};

struct Bus {
  Cartridge cart;
  Joypad joypad;

  COMPAT_MODE mode;

  // WRAM Bank
  u8 svbk = 0;

  // VRAM Bank
  u8 vbk  = 0;

  // BCPS
  PaletteSpecification bcps;
  PaletteSpecification ocps;

  std::array<RAM, 2> vram_banks = {RAM(0x2000), RAM(0x2000)};
  std::array<RAM, 8> wram_banks = {RAM(0x1000), RAM(0x1000), RAM(0x1000),
                                   RAM(0x1000), RAM(0x1000), RAM(0x1000),
                                   RAM(0x1000), RAM(0x1000)};

  RAM* vram = &vram_banks[0];
  RAM* wram = &wram_banks[1];

  RAM oam{0xA0};
  RAM io{0x100};
  RAM hram{0x80};
  RAM bg_palette_ram{0x40};
  RAM obj_palette_ram{0x40};

  void request_interrupt(InterruptType);
  [[nodiscard]] bool interrupt_pending() const;
  [[nodiscard]] std::string get_mode_string() const;
  u16 serial_port_index = 0;
  std::array<char, 0xFFFF> serial_port_buffer;
};