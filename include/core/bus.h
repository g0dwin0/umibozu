#pragma once
#include "cart.h"
#include "common.h"
#include "joypad.h"
#include "apu.h"
using namespace Umibozu;

enum class InterruptType {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};

enum class SYSTEM_MODE { DMG, CGB = 0xC0 };

struct PaletteSpecification {
  u8 address : 6      = 0x0;
  bool auto_increment = false;
};

struct Bus {
  SYSTEM_MODE mode;
  
  Cartridge cart;
  Joypad joypad;
  APU* apu = nullptr;

  // WRAM Bank
  u8 svbk = 0;

  // VRAM Bank
  u8 vbk = 0;

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
  
  // WAVE RAM
  RAM wave_ram{0x40};
  u8 wave_last_written_value = 0xFF;
  
  void request_interrupt(InterruptType);
  [[nodiscard]] bool interrupt_pending() const;
  [[nodiscard]] std::string get_mode_string() const;
  u16 serial_port_index = 0;
  std::array<char, 0xFFFF> serial_port_buffer;
};