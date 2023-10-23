#include "core/cpu.h"

#include <bitset>
#include <stdexcept>

#include "core/cart.h"

using namespace Umibozu;

SharpSM83::SharpSM83() { wram.resize(8192, 0); }
SharpSM83::~SharpSM83() {}

void SharpSM83::m_cycle() { cycles += 4; }

u8 SharpSM83::read8(const u16 address) {
  m_cycle();
  if (address >= 0 && address <= 0x3FFF) {
    return bus->cart.read8(address);
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    return bus->cart.read8((0x4000 * bus->cart.rom_bank) + address);
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
// debug only!
u8 SharpSM83::peek(const u16 address) {
  if (address >= 0 && address <= 0x3FFF) {
    return bus->cart.read8(address);
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    return bus->cart.read8((0x4000 * bus->cart.rom_bank) + address);
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
  if (address >= 0x0 && address <= 0x7FFF) {
    address >= 0x2000 ? (bus->cart.rom_bank = value & 0b00000111) : 0;
    // return bus->cart->write8(address, value);
  }
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

u8 SharpSM83::pull_from_stack() {return read8(SP++); }

void SharpSM83::set_flag(FLAG flag) {
  auto bitset = std::bitset<16>(AF);

  bitset.set((u8)flag);

  AF = bitset.to_ulong();
};

void SharpSM83::unset_flag(FLAG flag) {
  auto bitset = std::bitset<16>(AF);
  bitset.reset((u8)flag);
  AF = bitset.to_ulong();
};

u8 SharpSM83::get_flag(FLAG flag) {
  auto bitset = std::bitset<16>(AF);
  return bitset.test((size_t)flag);
}
void SharpSM83::set_zero() { set_flag(FLAG::ZERO); };
void SharpSM83::set_negative() { set_flag(FLAG::NEGATIVE); };
void SharpSM83::set_half_carry() { set_flag(FLAG::HALF_CARRY); };
void SharpSM83::set_carry() { set_flag(FLAG::CARRY); };

void SharpSM83::reset_zero() { unset_flag(FLAG::ZERO); };
void SharpSM83::reset_negative() { unset_flag(FLAG::NEGATIVE); };
void SharpSM83::reset_half_carry() { unset_flag(FLAG::HALF_CARRY); };
void SharpSM83::reset_carry() { unset_flag(FLAG::CARRY); };
void SharpSM83::run_instruction() {
  fmt::println(
      "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} "
      "L: {:02X} SP: {:02X} PC: {:02X}:{:04X} ({:02X} {:02X} {:02X} {:02X})",
      A, (AF & 0b11111111), B, C, D, E, H, L, SP, bus->cart.rom_bank, PC,
      peek(PC), peek(PC + 1), peek(PC + 2), peek(PC + 3));
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
      if (B == 0b1111) {
        set_half_carry();
      }
      B++;
      reset_negative();
      break;
    }
    case 0x5: {
      // half carry?
      B--;
      if (B == 0) {
        set_zero();
      }
      set_negative();
      break;
    }
    case 0x6: {
      B = read8(PC);
      PC++;
      break;
    }
    case 0x0D: {
      if (((C & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      C--;

      if (C == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }
    case 0xE: {
      C = read8(PC++);
      break;
    }
    case 0x11: {
      E  = read8(PC++);
      D  = read8(PC++);
      DE = (D << 8) + E;

      break;
    }
    case 0x12: {
      write8(DE, A);
      break;
    }
    case 0x14: {
      if (((D & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      D++;

      if (D == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      break;
    }
    case 0x18: {
      i8 offset = (i8)read8(PC++);
      m_cycle();
      PC = PC + offset;
      break;
    }
    case 0x1C: {
      if (((E & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      E++;
      // fmt::println("E: {:d}", E);

      if (E == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      break;
    }
    case 0x20: {
      i8 offset = (i8)read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        PC = PC + offset;
        m_cycle();
      }
      break;
    }
    case 0x21: {
      L  = read8(PC++);
      H  = read8(PC++);
      HL = (H << 8) + L;
      break;
    }
    case 0x26: {
      H = read8(PC++);
      break;
    }
    case 0x2A: {
      A = read8(HL++);
      
      H = (HL & 0xFF00) >> 8;
      L = (HL & 0xFF);

      break;
    }
    case 0x2F: {
      A = A ^ 0xFF;
      set_negative();
      set_half_carry();
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
    case 0x47: {
      B = A;
      break;
    }
    case 0x57: {
      D = A;
      break;
    }
    case 0x5A: {
      E = D;
      break;
    }
    case 0x5E: {
      E = read8(HL);
      break;
    }
    case 0x67: {
      H  = A;
      HL = (H << 8) + L;
      break;
    }
    case 0x6f: {
      L = A;
      break;
    }
    // case 0x70: {
    //   write8(HL, B);
    //   break;
    // }
    // case 0x71: {
    //   write8(HL, C);
    //   break;
    // }
    // case 0x72: {
    //   write8(HL, D);
    //   break;
    // }
    // case 0x73: {
    //   write8(HL, E);
    //   break;
    // }
    // case 0x74: {
    //   write8(HL, H);
    //   break;
    // }
    // case 0x75: {
    //   write8(HL, L);
    //   break;
    // }
    // case 0x77: {
    //   write8(HL, A);
    //   break;
    // }
    
    case 0x78: {
      A = B;
      break;
    }
    case 0x79: {
      A = C;
      break;
    }
    case 0x7A: {
      A = D;
      break;
    }
    case 0x7B: {
      A = E;
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
    case 0x7E: {
      A = read8(HL);
      break;
    }
    
    case 0xB9: {
      if ((A - C) < 0) {
        set_carry();
      }
      if ((A - C) == 0)
        set_zero();

      set_negative();
      break;
    }
    case 0xC1: {
      C  = pull_from_stack();
      B  = pull_from_stack();
      BC = (B << 8) + C;
      break;
    }
    case 0xC2: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC3: {
      PC = (read8(PC + 1) << 8) + read8(PC);
      m_cycle();
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
      push_to_stack((PC & 0xFF00) >> 8);
      push_to_stack(PC & 0xFF);

      u8 low  = read8(PC++);
      u8 high = read8(PC++);

      PC = (high << 8) + low;
      m_cycle();

      break;
    }
    case 0xCE: {
      reset_negative();
      u8 val = read8(PC++);
      if ((A & 16) == 0 && (A + val + get_flag(FLAG::CARRY) & 16))
        set_half_carry();
      if (A + val + get_flag(FLAG::CARRY) > 255)
        set_carry();
      A = A + val + get_flag(FLAG::CARRY);
      if (A == 0)
        set_zero();
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
      H  = (HL & 0xFF00) >> 8;
      L  = HL & 0xFF;
      m_cycle();
      break;
    }
    case 0xFB: {
      IME = true;
      break;
    }
    case 0xFF: {
      m_cycle();
      push_to_stack((PC & 0xFF00) >> 8);
      push_to_stack(PC & (0xFF));
      PC = 0x38;
      break;
    }
    default: {
      throw std::runtime_error(
          fmt::format("[CPU] unimplemented opcode: {:#04x}", opcode));
    }
  }
}