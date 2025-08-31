#pragma once
struct APU;
#include "apu.hpp"
#include "cart.hpp"
#include "common.hpp"
#include "io_defs.hpp"
#include "joypad.hpp"
#include "ppu.hpp"
struct Timer;
#include "timer.hpp"
struct PPU;
#include "mapper.hpp"
#include "ppu.hpp"

using namespace Umibozu;

enum class INTERRUPT_TYPE : u8 {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};

enum class SYSTEM_MODE : u8 { DMG, CGB = 0xC0 };

union PaletteSpecification {
  u8 v;
  struct {
    u8 address          : 6 = 0x0;
    u8                  : 1;
    bool auto_increment : 1 = false;
  };
};

struct Bus {
  SYSTEM_MODE mode = SYSTEM_MODE::DMG;

  Joypad joypad;
  Cartridge* cart = nullptr;
  PPU* ppu        = nullptr;
  Timer* timer    = nullptr;
  Mapper* mapper  = nullptr;
  APU* apu        = nullptr;
  // WRAM Bank
  u8 svbk = 0;

  // VRAM Bank
  u8 vbk = 0;

  bool cpu_is_halted = false;

  // BCPS
  PaletteSpecification bcps = {};
  PaletteSpecification ocps = {};

  std::array<std::array<u8, 0x2000>, 2> vram_banks = {};
  std::array<std::array<u8, 0x1000>, 8> wram_banks = {};

  std::array<u8, 0x2000>* vram = &vram_banks[0];
  std::array<u8, 0x1000>* wram = &wram_banks[1];

  std::array<u8, 0xA0> oam;
  std::array<u8, 0x100> io;
  std::array<u8, 0x80> hram;
  std::array<u8, 0x40> bg_palette_ram;
  std::array<u8, 0x40> obj_palette_ram;

  bool hidden_stat = should_raise_mode_0() || should_raise_mode_1() || should_raise_mode_2() || should_raise_ly_lyc();

  void update_hidden_stat() { hidden_stat = should_raise_mode_0() || should_raise_mode_1() || should_raise_mode_2() || should_raise_ly_lyc(); }

  std::array<u8, 0x10> wave_ram;
  u8 wave_last_written_value = 0xFF;

  void request_interrupt(INTERRUPT_TYPE);
  [[nodiscard]] bool interrupt_pending() const;
  [[nodiscard]] std::string get_mode_string() const;

  // Serial IO
  u16 serial_port_index = 0;
  std::array<char, 0xFFFF> serial_port_buffer;

  bool should_raise_mode_0() const;
  bool should_raise_mode_1() const;
  bool should_raise_mode_2() const;
  bool should_raise_ly_lyc() const;

  [[nodiscard]] std::string get_label(u16 addr);

  u8 read8(const u16 address);
  void write8(const u16 address, const u8 value);

  u8 io_read(const u16 address);
  void io_write(const u16 address, const u8 value);

  void init_hdma(u16 length);
  void terminate_hdma();
  void reset();

  bool double_speed_mode = false;
};