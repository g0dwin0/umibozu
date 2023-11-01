#pragma once
#include "cart/cart.h"
#include "common.h"

using namespace Umibozu;

struct RAM {
  std::vector<u8> ram;

  u8 read8(const u16 address);
  void write8(const u16 address, u8 value);

  RAM(size_t size) { ram.resize(size, 0); }
};
struct Bus {
  Cartridge cart;
  RAM ram = RAM(0x10000);
};