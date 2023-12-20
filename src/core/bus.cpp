#include "core/bus.h"

#include "common.h"

void RAM::write8(const u16 address, u8 value) { data[address] = value; }

u8 RAM::read8(const u16 address) { return data[address]; }

void Bus::request_interrupt(InterruptType t) { io.data[IF] |= (1 << (u8)t); }

std::string Bus::get_mode_string() const {
  if (mode == CGB_ONLY) {
    return "CGB ONLY";
  } else {
    return "DMG";
  }
}