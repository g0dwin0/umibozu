#pragma once
#include "cart.h"
#include "common.h"
using namespace Umibozu;

struct Joypad {
  u8 A      : 1 = 1;
  u8 B      : 1 = 1;
  u8 SELECT : 1 = 1;
  u8 START  : 1 = 1;

  u8 RIGHT  : 1 = 1;
  u8 LEFT   : 1 = 1;
  u8 UP     : 1 = 1;
  u8 DOWN   : 1 = 1;

  u8 get_buttons() const { return A + (B << 1) + (SELECT << 2) + (START << 3); }

  u8 get_dpad() const { return RIGHT + (LEFT << 1) + (UP << 2) + (DOWN << 3); }
};

enum struct InterruptType : u8 {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};
struct PaletteSpecification {
  u8 address : 6      = 0x0;
  bool auto_increment = false;
};
enum COMPAT_MODE {
  DMG,
  CGB_ONLY = 0xC0
};
struct Bus {
  Cartridge cart;
  Joypad joypad;
  
  COMPAT_MODE mode;

  // WRAM Bank
  u8 svbk : 3 = 0;

  // VRAM Bank
  u8 vbk  : 1 = 0;

  // BCPS
  PaletteSpecification bcps;
  PaletteSpecification ocps;

  // RAM wram{0x8000};
  std::array<RAM, 2> vram_banks = {RAM(0x2000), RAM(0x2000)};
  RAM* vram                     = &vram_banks[0];

  std::array<RAM, 8> wram_banks = {RAM(0x1000), RAM(0x1000), RAM(0x1000),
                                   RAM(0x1000), RAM(0x1000), RAM(0x1000),
                                   RAM(0x1000), RAM(0x1000)};
  RAM* wram                    = &wram_banks[1];
  RAM oam{0xA0};
  RAM io{0x100};
  RAM hram{0x80};
  RAM bg_palette_ram{0x40};
  RAM obj_palette_ram{0x40};

  void request_interrupt(InterruptType);
  std::string get_mode_string() const;
  u16 serial_port_index = 0;
  std::array<char, 0xFFFF> serial_port_buffer;
};