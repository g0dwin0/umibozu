#pragma once
#define FMT_HEADER_ONLY
#include <fmt/core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

enum HARDWARE_REG {
  P1   = 0xFF00,
  SB   = 0xFF01,
  SC   = 0xFF02,
  DIV  = 0xFF04,
  TIMA = 0xFF05,
  TMA  = 0xFF06,
  TAC  = 0xFF07,
  IF   = 0xFF0F,
  LCDC = 0xFF40,
  STAT = 0xFF41,
  SCY  = 0xFF42,
  SCX  = 0xFF43,
  LY   = 0xFF44,
  LYC  = 0xFF45,
  DMA  = 0xFF46,
  BGP  = 0xFF47,
  OBP0 = 0xFF48,
  OBP1 = 0xFF49,
  WY   = 0xFF4A,
  WX   = 0xFF4B,
  IE   = 0xFFFF
};

// TODO: move these to where they actually belong
enum struct InterruptType {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};
struct RAM {
  std::vector<u8> data;
  u8 read8(const u16 address);
  void write8(const u16 address, u8 value);
  RAM(size_t size) { data.resize(size, 0); }
};