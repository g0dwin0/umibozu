#pragma once
#include "cart.h"
#include "common.h"
#include "ppu.h"
using namespace Umibozu;

struct Bus {
  Cartridge cart;
  PPU ppu;
  RAM wram = RAM(0x10000);
  RAM vram = RAM(0x2000);

  // Game Link Port
  u16 serial_port_index = 0;
  char serial_port_buffer[0xFFFF];
};