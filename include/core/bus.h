#pragma once
#include "cart.h"
#include "common.h"
using namespace Umibozu;

struct GCM {
  u8 A : 1      = 1;
  u8 B : 1      = 1;
  u8 SELECT : 1 = 1;
  u8 START : 1  = 1;

  u8 RIGHT : 1 = 1;
  u8 LEFT : 1  = 1;
  u8 UP : 1    = 1;
  u8 DOWN : 1  = 1;

  u8 get_buttons() { return A + (B << 1) + (SELECT << 2) + (START << 3); }

  u8 get_dpad() { return RIGHT + (LEFT << 1) + (UP << 2) + (DOWN << 3); }
};

enum struct InterruptType {
  VBLANK,
  LCD,
  TIMER,
  SERIAL,
  JOYPAD,
};

struct Bus {
  Cartridge cart;
  GCM control_manager;

  RAM wram = RAM(0x10000);
  RAM vram = RAM(0x2000);
  RAM oam  = RAM(0xA0);

  void request_interrupt(InterruptType);
  u16 serial_port_index = 0;
  char serial_port_buffer[0xFFFF];
};