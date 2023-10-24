#include "core/cpu.h"

#include <bitset>
#include <stdexcept>

#include "core/cart.h"
#include "fmt/core.h"

using namespace Umibozu;

SharpSM83::SharpSM83() {}
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
    if (address == 0xFF44)
      return 0x90;
    return bus->ram.read8(address);
    exit(-1);
    throw std::runtime_error(
        fmt::format("reading register address space: {:#04x}", address));
  }
  if (address == 0xFFFF) {
    // fmt::println("IME: {:#4x}", IME);
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
    return bus->cart.read8(address);
  }
  if (address >= 0x8000 && address <= 0xDFFF) {
    return bus->ram.read8(address);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->ram.read8((address & 0xDDFF));
  }
  if (address >= 0xFE00 && address <= 0xFFFE) {
    if (address >= 0xFE0 && address <= 0xFEFF) {
      return 0;
    }
    return bus->ram.read8(address);
  }
  if (address == 0xFFFF) {
    // fmt::println("IME: {:#4x}", IME);
    return IME;
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}

// void SharpSM83::init_hw_regs() {
//   dfgknndklk
// }
void SharpSM83::write8(const u16 address, const u8 value) {
  m_cycle();
  if (address >= 0x0 && address <= 0x7FFF) {
    address >= 0x2000 ? (bus->cart.rom_bank = value & 0b00000111) : 0;
    throw std::runtime_error("write to ROM area/MBC register");
  }
  if (address >= 0x8000 && address <= 0xDFFF) {
    return bus->ram.write8(address, value);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->ram.write8((address & 0xDDFF), value);
  }
  if (address >= 0xFE00 && address <= 0xFFFE) {
    // throw std::runtime_error("handle dat");
    return bus->ram.write8(address, value);
  }
  if (address == 0xFFFF) {
    IME = value;
    return;
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
}
void SharpSM83::push_to_stack(const u8 value) { write8(--SP, value); }

u8 SharpSM83::pull_from_stack() { return read8(SP++); }

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
      A, (AF & 0xFF), B, C, D, E, H, L, SP, bus->cart.rom_bank, PC, peek(PC),
      peek(PC + 1), peek(PC + 2), peek(PC + 3));
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
    case 0x3: {
      m_cycle();
      BC++;
      B = (BC & 0xFF00) >> 8;
      C = (BC & 0xFF);
      break;
    }
    case 0x4: {
      if (((B & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      B++;
      if (B == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
      break;
    }
    case 0x5: {
      if (((B & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      B--;
      if (B == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      set_negative();
      break;
    }
    case 0x6: {
      B = read8(PC);
      PC++;
      break;
    }
    case 0xD: {
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
    case 0x13: {
      m_cycle();
      DE++;
      D = (DE & 0xFF00) >> 8;
      E = (DE & 0xFF);
      break;
    }
    case 0x14: {
      if (((D & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      D++;
      DE = (D << 8) + E;
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
    case 0x1A: {
      A = read8(DE);
      break;
    }
    case 0x1C: {
      if (((E & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      E++;
      DE = (D << 8) + E;
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
    case 0x22: {
      write8(HL++, A);

      H  = (HL & 0xFF00) >> 8;
      L  = (HL & 0xFF);
      AF = (A << 8) + (AF & 0xFF);
      break;
    }

    case 0x23: {
      m_cycle();
      HL++;
      H = (HL & 0xFF00) >> 8;
      L = (HL & 0xFF);
      break;
    }
    case 0x24: {
      if (((H & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      H++;
      HL = (H << 8) + L;
      if (H == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      break;
    }

    case 0x26: {
      H = read8(PC++);
      break;
    }
    case 0x28: {
      i8 offset = (i8)read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x2A: {
      A = read8(HL++);

      H  = (HL & 0xFF00) >> 8;
      L  = (HL & 0xFF);
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x2D: {
      if (((L & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      L--;
      HL = (H << 8) + L;
      if (L == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }
    case 0x2C: {
      if (((L & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      L++;
      HL = (H << 8) + L;
      if (L == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
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
    case 0x32: {
      write8(HL--, A);

      H  = (HL & 0xFF00) >> 8;
      L  = (HL & 0xFF);
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x33: {
      m_cycle();
      SP++;
      break;
    }
    case 0x3C: {
      A++;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x3E: {
      A  = read8(PC++);
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x40: {
      break;
    }
    case 0x41: {
      B  = C;
      BC = (B << 8) + C;
      break;
    }

    case 0x42: {
      B  = D;
      BC = (B << 8) + C;
      break;
    }
    case 0x43: {
      B  = E;
      BC = (B << 8) + C;
      break;
    }
    case 0x44: {
      B  = H;
      BC = (B << 8) + C;
      break;
    }
    case 0x45: {
      B  = L;
      BC = (B << 8) + C;
      break;
    }
    case 0x46: {
      B  = read8(HL);
      BC = (B << 8) + C;
      break;
    }
    case 0x47: {
      B  = A;
      BC = (B << 8) + C;
      break;
    }

    case 0x48: {
      C  = B;
      BC = (B << 8) + C;
      break;
    }
    case 0x49: {
      break;
    }
    case 0x4A: {
      C  = D;
      BC = (B << 8) + C;
      break;
    }
    case 0x4B: {
      C  = E;
      BC = (B << 8) + C;
      break;
    }
    case 0x4C: {
      C  = H;
      BC = (B << 8) + C;
      break;
    }
    case 0x4D: {
      C  = L;
      BC = (B << 8) + C;
      break;
    }
    case 0x4E: {
      C  = read8(HL);
      BC = (B << 8) + C;
      break;
    }
    case 0x4F: {
      C  = H;
      BC = (B << 8) + C;
      break;
    }

    case 0x50: {
      D  = B;
      DE = (D << 8) + E;
      break;
    }
    case 0x51: {
      D  = C;
      DE = (D << 8) + E;
      break;
    }
    case 0x52: {
      break;
    }
    case 0x53: {
      D  = E;
      DE = (D << 8) + E;
      break;
    }
    case 0x54: {
      D  = H;
      DE = (D << 8) + E;
      break;
    }
    case 0x55: {
      D  = L;
      DE = (D << 8) + E;
      break;
    }
    case 0x56: {
      D  = read8(HL);
      DE = (D << 8) + E;
      break;
    }
    case 0x57: {
      D  = A;
      DE = (D << 8) + E;
      break;
    }
    case 0x58: {
      E  = B;
      DE = (D << 8) + E;
      break;
    }
    case 0x59: {
      E  = C;
      DE = (D << 8) + E;
      break;
    }
    case 0x5A: {
      E  = D;
      DE = (D << 8) + E;
      break;
    }
    case 0x5B: {
      break;
    }
    case 0x5C: {
      E  = H;
      DE = (D << 8) + E;
      break;
    }
    case 0x5D: {
      E  = L;
      DE = (D << 8) + E;
      break;
    }
    case 0x5E: {
      E  = read8(HL);
      DE = (D << 8) + E;
      break;
    }
    case 0x5F: {
      E  = A;
      DE = (D << 8) + E;
      break;
    }
    case 0x66: {
      H  = read8(HL);
      HL = (H << 8) + L;
      break;
    }
    case 0x67: {
      H  = A;
      HL = (H << 8) + L;
      break;
    }
    case 0x6c: {
      L  = H;
      HL = (H << 8) + L;
      break;
    }
    case 0x6f: {
      L  = A;
      HL = (H << 8) + L;
      break;
    }
    case 0x70: {
      write8(HL, B);
      break;
    }
    case 0x71: {
      write8(HL, C);
      break;
    }
    case 0x72: {
      write8(HL, D);
      break;
    }
    case 0x73: {
      write8(HL, E);
      break;
    }
    case 0x74: {
      write8(HL, H);
      break;
    }
    case 0x75: {
      write8(HL, L);
      break;
    }
    case 0x77: {
      write8(HL, A);
      break;
    }

    case 0x78: {
      A  = B;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x79: {
      A  = C;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x7A: {
      A  = D;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x7B: {
      A  = E;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x7C: {
      A  = H;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x7D: {
      A  = L;
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x7E: {
      A  = read8(HL);
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0x93: {
      if ((A - E) < 0) {
        set_carry();
      }
      A = A - E;
      if (A == 0) {
        set_zero();
      }
      set_negative();
      AF = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0xA0: {
      A = A & B;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA1: {
      A = A & C;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA2: {
      A = A & D;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA3: {
      A = A & E;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA5: {
      A = A & L;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA6: {
      A = A & read8(HL);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }
    case 0xA7: {
      A = A & A;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
      break;
    }

    case 0xA8: {
      A = A ^ B;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xA9: {
      A = A ^ C;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAA: {
      A = A ^ D;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAB: {
      A = A ^ E;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAC: {
      A = A ^ H;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAD: {
      A = A ^ L;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAE: {
      A = A ^ read8(HL);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xAF: {
      A = A ^ A;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }

    case 0xB0: {
      A  = A | B;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB1: {
      A  = A | C;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB2: {
      A  = A | D;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB3: {
      A  = A | E;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB4: {
      A  = A | H;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB5: {
      A  = A | L;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB6: {
      A  = A | read8(HL);
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }

    case 0xB7: {
      A  = A | A;
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
      break;
    }
    case 0xB8: {
      if ((A - B) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      if (((A & 0xf) - (B & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A - B) == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }

    case 0xB9: {
      if ((A - C) < 0) {
        set_carry();
      }
      if ((A - C) == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }
    case 0xBA: {
      if ((A - D) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      if (((A & 0xf) - (D & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A - D) == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }

    case 0xBB: {
      if ((A - E) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      if (((A & 0xf) - (E & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A - E) == 0) {
        set_zero();
      } else {
        reset_zero();
      }

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
      u8 low  = read8(PC++);
      u8 high = read8(PC++);

      PC = (high << 8) + low;
      m_cycle();
      break;
    }
    case 0xC4: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        m_cycle();
      }
      break;
    }
    case 0xC5: {
      m_cycle();
      push_to_stack(B);
      push_to_stack(C);
      break;
    }
    case 0xC6: {
      u8 vl = read8(PC++);
      if (((A & 0xf) + (vl & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A + vl) > 255) {
        set_carry();
      } else {
        reset_carry();
      }
      A += vl;
      AF = (A << 8) + (AF & 0xFF);
      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      break;
    }
    case 0xC7: {
      m_cycle();
      push_to_stack((PC & 0xFF00) >> 8);
      push_to_stack((PC & 0xFF));
      PC = 0x0;
      break;
    }
    case 0xC9: {
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      m_cycle();
      PC = (high << 8) + low;
      // exit(0);
      break;
    }
    case 0xCB: {
      switch(read8(PC++)) {
        case 0x38: {
          if(B & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          B >>= 1;
          BC = (B << 8) + C;
           if(B == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x39: {
          if(C & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          C >>= 1;
          BC = (B << 8) + C;
           if(C == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3A: {
          if(D & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          D >>= 1;
          DE = (D << 8) + E;
           if(D == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3B: {
          if(E & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          E >>= 1;
          DE = (D << 8) + E;
           if(E == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3C: {
         if(H & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          H >>= 1;
          if(H == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          HL = (H << 8) + L;
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3D: {
         if(L & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          L >>= 1;
          if(L == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          HL = (H << 8) + L;
          reset_negative();
          reset_half_carry();
          break;
        }
        // case 0x3E: {
        //   if(B & 0x80) {
        //     set_carry();
        //   } else {
        //     reset_carry();
        //   }
        //   B >>= 1;
        //   reset_negative();
        //   reset_half_carry();
        //   break;
        // }
        // case 0x3F: {
        //   if(B & 0x80) {
        //     set_carry();
        //   } else {
        //     reset_carry();
        //   }
        //   B >>= 1;
        //   reset_negative();
        //   reset_half_carry();
        //   break;
        // }
        
        
        default: {
          fmt::println("[CPU] unimplemented CB op: {:#04x}", peek(PC - 1));
          exit(-1);
        }
      }
      break;
    }
    case 0xCD: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);

      push_to_stack(((PC & 0xFF00) >> 8));
      push_to_stack((PC & 0xFF));

      PC = (high << 8) + low;
      m_cycle();

      break;
    }
    case 0xCE: {
      u8 val = read8(PC++);

      if (((A & 0xf) + ((val + get_flag(FLAG::CARRY)) & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if (A + val + get_flag(FLAG::CARRY) > 255) {
        set_carry();
      } else {
        reset_carry();
      }
      A = A + val + get_flag(FLAG::CARRY);
      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
      break;
    }
    case 0xD5: {
      m_cycle();
      push_to_stack(D);
      push_to_stack(E);
      break;
    }
    case 0xD6: {
      u8 val = read8(PC++);
      if (((A & 0xf) - ((val)&0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if((A - val) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      A -= val;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      AF = (A << 8) + (AF & 0xFF);

      set_negative();
      break;
    }
    case 0xE0: {
      u16 address = 0xFF00 + read8(PC++);
      // fmt::println("address: {:04x}", address);
      write8(address, A);
      break;
    }

    case 0xE1: {
      L  = pull_from_stack();
      H  = pull_from_stack();
      HL = (H << 8) + L;
      break;
    }
    case 0xE5: {
      m_cycle();
      push_to_stack(H);
      push_to_stack(L);
      break;
    }
    case 0xE6: {
      A  = A & read8(PC++);
      AF = (A << 8) + (AF & 0xFF);

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_half_carry();

      reset_negative();
      reset_carry();
      break;
    }
    case 0xEA: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      write8(((high << 8) + low), A);
      break;
    }
    case 0xF0: {
      A = read8(0xFF00 + read8(PC++));
      break;
    }
    case 0xF1: {
      u8 f = pull_from_stack();
      A    = pull_from_stack();
      AF   = (A << 8) + f;

      break;
    }
    case 0xF3: {
      IME = false;
      break;
    }
    case 0xFA: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      A       = read8((high << 8) + low);
      AF      = (A << 8) + (AF & 0xFF);
      break;
    }
    case 0xF5: {
      m_cycle();
      push_to_stack(A);
      push_to_stack(AF & 0xFF);
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
    case 0xFE: {
      u8 val = read8(PC++);
      if ((A - val) < 0) {
        set_carry();
      }
      if (((A & 0xf) - (val & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A - val) == 0)
        set_zero();

      set_negative();
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
      fmt::println("[CPU] unimplemented opcode: {:#04x}", opcode);
      ;
      exit(0);
      throw std::runtime_error(
          fmt::format("[CPU] unimplemented opcode: {:#04x}", opcode));
    }
  }
}