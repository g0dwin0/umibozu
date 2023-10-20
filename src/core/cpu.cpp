#include "core/cpu.h"

#include "core/cart.h"
#include "core/instruction_table.h"

using namespace Umibozu;

SharpSM83::SharpSM83() { wram.resize(8192, 0); }
SharpSM83::~SharpSM83() {}

u8 SharpSM83::read8(const u16 address) {
  if (address >= 0 && address <= 0x3FFF) {
    return bus->cart->read8(address);
  }
  throw std::runtime_error(fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));

  return 0;
}
void SharpSM83::write8(const u16 address, const u8 value) {
  if (address >= 0 && address <= 0x3FFF) {
    bus->cart->write8(address, value);
  }
  throw std::runtime_error(fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));

}

void SharpSM83::run_instruction() {
  fmt::println("PC: {:#X}", PC);
  u8 opcode = read8(PC++);
  Instruction instr = lookup_table[opcode];
  instr.func(this);
  fmt::println("opcode: {:#04x}", opcode);
}