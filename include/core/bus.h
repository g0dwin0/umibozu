#pragma once
#include "cart.h"
#include "common.h"
using namespace Umibozu;
struct RAM {
  std::vector<u8> ram;

  u8 read8(const u16 address) { return ram.at(address); }
  void write8(const u16 address, u8 value) {
    fmt::println("writing {:#04x} to {:#04x}", value, address);
    ram.at(address) = value;
  }

  RAM(size_t size) {
    fmt::println("[BUS] RAM size: {:#4x}", size);
    ram.resize(size, 0);
  }
};
struct Bus {
  Cartridge cart;
  RAM ram = RAM(0x10000);
};