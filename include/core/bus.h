#pragma once
#include "cart.h"
#include "common.h"

using namespace Umibozu;

struct Bus {
  Cartridge cart;
  RAM wram = RAM(0x10000);

  // Game Link Port
  u16 serial_port_index = 0;
  char serial_port_buffer[0xFFFF];
};