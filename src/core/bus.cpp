#include "core/bus.h"
#include "common.h"

void RAM::write8(const u16 address, u8 value) {
  data.at(address) = value;
}
u8 RAM::read8(const u16 address) { return data.at(address); }