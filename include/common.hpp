#pragma once
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

enum class PRIORITY_MODE { CGB, DMG };

enum IO_REG {
  JOYPAD      = 0x00,
  SB          = 0x01,
  SC          = 0x02,
  UNUSED_FF03 = 0x03,
  DIV         = 0x04,
  TIMA        = 0x05,
  TMA         = 0x06,
  TAC         = 0x07,
  IF          = 0x0F,
  NR10        = 0x10,
  NR11        = 0x11,
  NR12        = 0x12,
  NR13        = 0x13,
  NR14        = 0x14,
  NR21        = 0x16,
  NR22        = 0x17,
  NR23        = 0x18,
  NR24        = 0x19,
  NR30        = 0x1A,
  NR31        = 0x1B,
  NR32        = 0x1C,
  NR33        = 0x1D,
  NR34        = 0x1E,
  NR41        = 0x20,
  NR42        = 0x21,
  NR43        = 0x22,
  NR44        = 0x23,
  NR50        = 0x24,
  NR51        = 0x25,
  NR52        = 0x26,
  LCDC        = 0x40,
  STAT        = 0x41,
  SCY         = 0x42,
  SCX         = 0x43,
  LY          = 0x44,
  LYC         = 0x45,
  DMA         = 0x46,
  BGP         = 0x47,
  OBP0        = 0x48,
  OBP1        = 0x49,
  WY          = 0x4A,
  WX          = 0x4B,

  //  CGB
  KEY0  = 0x4C,  // backwards compat reg
  KEY1  = 0x4D,
  VBK   = 0x4F,
  HDMA1 = 0x51,
  HDMA2 = 0x52,
  HDMA3 = 0x53,
  HDMA4 = 0x54,
  HDMA5 = 0x55,
  RP    = 0x56,
  BCPS  = 0x68,
  BCPD  = 0x69,
  OCPS  = 0x6A,
  OCPD  = 0x6B,
  OPRI  = 0x6C,
  SVBK  = 0x70,
  PCM12 = 0x76,
  PCM34 = 0x77,

  IE = 0xFF
};
