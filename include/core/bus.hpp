#pragma once
#include <spdlog/sinks/stdout_color_sinks.h>

#include "apu.hpp"
#include "cart.hpp"
#include "common.hpp"
#include "joypad.hpp"
struct Mapper;
#include "mapper.hpp"
#include "timer.hpp"
struct PPU;
#include "ppu.hpp"

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

  Bus() : oam(0xA0), io(0x100), hram(0x80), bg_palette_ram(0x40), obj_palette_ram(0x40), wave_ram(0x40) {};

#ifdef CPU_TEST_MODE
  std::vector<u8> test_memory;
#endif

  Cartridge cart = {};
  Joypad joypad  = {};
  // APU* apu       = nullptr;
  PPU* ppu       = nullptr;
  Timer* timer   = nullptr;
  Mapper* mapper = nullptr;

  // WRAM Bank ID
  u8 svbk = 0;

  // VRAM Bank ID
  u8 vbk = 0;

  // BCPS
  PaletteSpecification bcps;
  PaletteSpecification ocps;

  std::array<std::vector<u8>, 2> vram_banks = {std::vector<u8>(0x2000), std::vector<u8>(0x2000)};
  std::array<std::vector<u8>, 8> wram_banks = {std::vector<u8>(0x1000), std::vector<u8>(0x1000), std::vector<u8>(0x1000), std::vector<u8>(0x1000),
                                               std::vector<u8>(0x1000), std::vector<u8>(0x1000), std::vector<u8>(0x1000), std::vector<u8>(0x1000)};

  std::vector<u8>* vram = &vram_banks[0];
  std::vector<u8>* wram = &wram_banks[1];

  std::vector<u8> oam;
  std::vector<u8> io;
  std::vector<u8> hram;
  std::vector<u8> bg_palette_ram;
  std::vector<u8> obj_palette_ram;

  // wave ram
  std::vector<u8> wave_ram;
  u8 wave_last_written_value = 0xFF;

  void request_interrupt(InterruptType);
  [[nodiscard]] bool interrupt_pending() const;
  [[nodiscard]] std::string get_mode_string() const;

  [[nodiscard]] u8 read8(const u16 address);
  [[nodiscard]] u16 read16(const u16 address);
  u8 peek(const u16 address) const;
  void write8(const u16 address, const u8 value);
  void push_to_stack(const u8 value);
  u8 pull_from_stack();
  void io_write(const u16 address, const u8 value);
  u8 io_read(const u16 address);

  void m_cycle();

#ifndef CPU_TEST_MODE
  std::shared_ptr<spdlog::logger> io_logger = spdlog::stdout_color_mt("IO");
#endif

  // serial comm (unused, some roms print text for debugging)
  u16 serial_port_index = 0;
  std::array<char, 0xFFFF> serial_port_buffer;
};