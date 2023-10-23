#include "core/cpu.h"

#include <stdexcept>

#include "core/cart.h"
#include "fmt/core.h"
#include "fmt/format.h"

using namespace Umibozu;

SharpSM83::SharpSM83() { wram.resize(8192, 0); }
SharpSM83::~SharpSM83() {}

void SharpSM83::m_cycle() { cycles += 4; }

u8 SharpSM83::read8(const u16 address) {
  m_cycle();

  if (address >= 0 && address <= 0x3FFF) {
    return bus->cart.read8(address);
  }
  if (address >= 0x8000 && address <= 0xDFFF) {
    return bus->ram.read8(address);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->ram.read8((address & 0xDDFF));
  }
  if (address >= 0xFE00 && address <= 0xFFFE) {
    return bus->ram.read8(address);
  }
  if (address == 0xFFFF) {
    fmt::println("IME: {:#4x}", IME);
    return IME;
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}
void SharpSM83::write8(const u16 address, const u8 value) {
  m_cycle();
  // if (address >= 0x0 && address <= 0x7FFF) {
  //   address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;
  //   return bus->cart->write8(address, value);
  // }
  if (address >= 0x8000 && address <= 0xDFFF) {
    return bus->ram.write8(address, value);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->ram.write8((address & 0xDDFF), value);
  }
  if (address >= 0xFE00 && address <= 0xFFFE) {
    return bus->ram.write8(address, value);
  }
  if (address == 0xFFFF) {
    IME = value;
    return;
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
}
void SharpSM83::push_to_stack(const u8 value) { write8(SP--, value); }

u8 SharpSM83::pull_from_stack() { return read8(SP++); }

void SharpSM83::run_instruction() {
  u8 opcode = read8(PC++);
  switch (opcode) {
    case 0x0: {
      break;
    }
    case 0x1: {
      C  = read8(PC++);
      B  = read8(PC++);
      BC = (B << 8) + C;
      break;
    }
    case 0x4: {
      B++;
      break;
    }
    case 0x5: {
      B--;
      break;
    }
    case 0x6: {
      B = read8(PC);
      PC++;
      break;
    }
    case 0x21: {
      H  = read8(PC++);
      L  = read8(PC++);
      HL = (H << 8) + L;
      break;
    }
    case 0x31: {
      SP = (read8(PC + 1) << 8) + read8(PC);
      PC += 2;
      break;
    }
    case 0x3C: {
      A++;
      break;
    }
    case 0x3E: {
      A = read8(PC++);
      break;
    }
    case 0x57: {
      D = A;
      break;
    }
    case 0x5E: {
      E = read8(HL);
      break;
    }
    case 0x67: {
      H = A;
      break;
    }
    case 0x7A: {
      A = D;
      break;
    }
    case 0x7C: {
      A = H;
      break;
    }
    case 0x7D: {
      A = L;
      break;
    }
    case 0xC1: {
      C  = pull_from_stack();
      B  = pull_from_stack();
      BC = (B << 8) + C;
      break;
    }
    case 0xC3: {
      PC = (read8(PC + 1) << 8) + read8(PC);
      m_cycle();
      // PC += 2;
      break;
    }
    case 0xC9: {
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      PC      = (high << 8) + low;
      m_cycle();
      break;
    }
    case 0xCD: {
      push_to_stack(PC & 0xFF00);
      push_to_stack(PC & 0xFF);

      u8 low  = read8(PC++);
      u8 high = read8(PC++);

      PC = (high << 8) + low;
      m_cycle();

      break;
    }
    case 0xD6: {
      A -= read8(PC++);
      break;
    }
    case 0xE0: {
      write8(0xFF00 + read8(PC++), A);
      break;
    }
    case 0xEA: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      write8((high << 8) + low, A);
      break;
    }
    case 0xFA: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      A       = read8((high << 8) + low);
      break;
    }
    case 0xF3: {
      IME = false;
      break;
    }
    case 0xF9: {
      SP = HL;
      H  = HL & 0xFF00;
      L  = HL & 0xFF;
      m_cycle();
      break;
    }
    case 0xFF: {
      m_cycle();
      push_to_stack(PC & (0xFF << 8));
      push_to_stack(PC & (0xFF));
      PC = 0x38;
      break;
    }
    default: {
      throw std::runtime_error(
          fmt::format("[CPU] unimplemented opcode: {:#04x}", opcode));
    }
  }
  fmt::println(
      "PC: {:#x}, SP: {:#x}, A: {:#x}, B: {:#x}, C: {:#x}, D: {:#x}, E: {:#x}, "
      "H: {:#x}, L: {:#x} RB: {:#x}",
      PC, SP, A, B, C, D, E, H, L, rom_bank);
}