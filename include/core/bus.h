#pragma once
#include "cart.h"
#include "common.h"
using namespace Umibozu;

struct Bus {
  Cartridge cart;

  RAM wram = RAM(0x10000);
  RAM vram = RAM(0x2000);
  RAM oam = RAM(0xA0);

  void request_interrupt(InterruptType);
  u16 serial_port_index = 0;
  char serial_port_buffer[0xFFFF];
};