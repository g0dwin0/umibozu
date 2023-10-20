#pragma once
#include "common.h"
#include "cart.h"
using namespace Umibozu;
struct Bus {
  Cartridge* cart;
  std::vector<u8> wram;

  Bus() {
    wram.resize(0xFFF, 0);
  }
  
};