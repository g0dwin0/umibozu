#include "core/cpu/cpu.h"

#include <bitset>
#include <stdexcept>

#include "common.h"
#include "core/cart/cart.h"
#include "fmt/core.h"

using namespace Umibozu;

SharpSM83::SharpSM83() {}
SharpSM83::~SharpSM83() {}

void SharpSM83::request_interrupt(InterruptType t) {
  bus->ram.ram[IF] |= (1 << (u8)t);
}
void SharpSM83::m_cycle() {
  bus->ram.ram[DIV] += 1;
  if (tima_to_tma) {
    bus->ram.ram[TIMA] = bus->ram.ram[TMA];
    tima_to_tma        = false;
  }
  if ((bus->ram.ram[TAC] & 0x4)) {
    if ((bus->ram.ram[TIMA] +
         (1'048'576 / (clock_select_table[(bus->ram.ram[TAC] & 0x3)]))) >
        0x100) {
      bus->ram.ram[TIMA] = 0;
      // fmt::println("TIMA: {:#04x}", bus->ram.ram[TIMA]);

      tima_to_tma = true;
      request_interrupt(InterruptType::TIMER);
      // fmt::println("timer interrupt requested...");
    } else {
      bus->ram.ram[TIMA] +=
          1'048'576 / (clock_select_table[(bus->ram.ram[TAC] & 0x3)]);
      // fmt::println("TIMA: {:#04x}", bus->ram.ram[TIMA]);
    }
  }
  // handle_interrupts();
}

u8 SharpSM83::read8(const u16 address) {
  m_cycle();
  
  if (address == 0xFF44) {
    return 0x90;
  }
  if (address <= 0x3FFF) {
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
    return bus->ram.ram[IE];
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}
u16 SharpSM83::read16(const u16 address) {
  u8 low  = read8(address);
  u8 high = read8(address + 1);
  PC++;
  return (high << 8) + low;
}
// debug only!
u8 SharpSM83::peek(const u16 address) {
  // if (address <= 0xFEFF) {
  //   return 0;
  // }
  if (address <= 0x3FFF) {
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
    return bus->ram.read8(address);
  }
  if (address == 0xFFFF) {
    return bus->ram.ram[IE];
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}

void SharpSM83::write8(const u16 address, const u8 value) {
  m_cycle();
  // fmt::println("[WRITE8] writing {:#04x} to {:#04x}", value, address);
  if (address <= 0x7FFF) {
    address >= 0x2000 ? (bus->cart.rom_bank = value & 0b00000111) : 0;
    return bus->ram.write8(address, value);
  }
  if (address >= 0x8000 && address <= 0xDFFF) {
    return bus->ram.write8(address, value);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->ram.write8((address & 0xDDFF), value);
  }
  if (address >= 0xFE00 && address <= 0xFFFE) {
    if (address >= 0xFF00 && address <= 0xFF4B) {
      switch (address) {
        case DIV: {
          bus->ram.ram[DIV] = 0x0;
          return;
        }
        case TMA: {
          // fmt::println("TMA value: {:08b}", value);
          bus->ram.ram[TMA] = value;
          return;
        }
        case TIMA: {
          // fmt::println("TIMA value: {:08b}", value);
          bus->ram.ram[TIMA] = value;
          return;
        }
        case TAC: {
          // fmt::println("TAC value: {:08b}", value);
          bus->ram.ram[TAC] = value;
          // fmt::println("new cycle increment count: {:d}",
          //              clock_select_table[value & 0x3]);
          return;
        }
        case IF: {
          bus->ram.ram[IF] = value;
          // fmt::println("IE: {:08b}", bus->ram.ram[IE]);
          // fmt::println("IF: {:08b}", bus->ram.ram[IF]);
          return;
        }
        case IE: {
          bus->ram.ram[IE] = value;
          // fmt::println("IE: {:08b}", bus->ram.ram[IE]);
          // fmt::println("IF: {:08b}", bus->ram.ram[IF]);
          return;
        }
        default: {
          // fmt::println("unhandled io reg write");
          break;
        }
      }
    }
  }
  return bus->ram.write8(address, value);
  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
}
void SharpSM83::push_to_stack(const u8 value) { write8(--SP, value); }

u8 SharpSM83::pull_from_stack() { return read8(SP++); }

void SharpSM83::set_flag(FLAG flag) { F |= (1 << (u8)flag); };

void SharpSM83::unset_flag(FLAG flag) { F &= ~(1 << (u8)flag); };

u8 SharpSM83::get_flag(FLAG flag) { return F & (1 << (u8)flag) ? 1 : 0; }

void SharpSM83::handle_interrupts() {
  if (IME && bus->ram.ram[IE] && bus->ram.ram[IF]) {
    // fmt::println("[HANDLE INTERRUPTS] hello!");
    fmt::println("[HANDLE INTERRUPTS] IME:  {:08b}", IME);
    fmt::println("[HANDLE INTERRUPTS] IE:   {:08b}", bus->ram.ram[IE]);
    fmt::println("[HANDLE INTERRUPTS] IF:   {:08b}", bus->ram.ram[IF]);

    // IE -> what specific interrupt handler is allowed to be called? (per bit)
    // IF -> requested interrupts (handle here!)

    std::bitset<8> ie_set(bus->ram.ram[IE]);
    std::bitset<8> if_set(bus->ram.ram[IF]);

    for (size_t i = 0; i < if_set.size(); i++) {
      if (if_set[i] == true && ie_set[i] == true) {
        IME = false;

        fmt::println("handling interrupt: {:#04x}", i);
        fmt::println("[INTERRUPT HANDLER] PC: {:#04x}", PC);
        fmt::println("{}", if_set.to_string());
        if_set[i].flip();
        fmt::println("{}", if_set.to_string());
        m_cycle();
        m_cycle();
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));
        PC = interrupt_table[i];
        m_cycle();
        fmt::println("[INTERRUPT HANDLER] AFTER PC: {:#04x}", PC);
      };
    }

    bus->ram.ram[IE] = ie_set.to_ulong();
    bus->ram.ram[IF] = if_set.to_ulong();
    // Bit 0 (VBlank) has the highest priority, and Bit 4 (Joypad) has the
    // lowest priority.
  }
};
void SharpSM83::run_instruction() {
  fmt::println(
      "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} "
      "L: {:02X} SP: {:04X} PC: {:02X}:{:04X} ({:02X} {:02X} {:02X} {:02X})",
      A, F, B, C, D, E, H, L, SP, bus->cart.rom_bank, PC, peek(PC),
      peek(PC + 1), peek(PC + 2), peek(PC + 3));
  u8 opcode = read8(PC++);
  switch (opcode) {
    case 0x0: {
      NOP();
      break;
    }
    case 0x1: {
      LD_R16_U16(BC, read16(PC++));
      break;
    }
    case 0x3: {
      INC_16(BC);
      break;
    }
    case 0x4: {
      INC(B);
      break;
    }
    case 0x5: {
      DEC(B);
      break;
    }
    case 0x6: {
      LD_R_R(B, read8(PC++));
      break;
    }
    case 0x7: {
      RLCA();
      break;
    }
    case 0x8: {
      LD_U16_SP(read16(PC++), SP);
      break;
    }
    case 0x9: {
      if (((HL & 0xfff) + (BC & 0xfff)) & 0x1000) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((HL + BC) > 0xFFFF) {
        set_carry();
      } else {
        reset_carry();
      }
      HL = HL + BC;
      H  = (HL & 0xFF00) >> 8;
      L  = (HL & 0xFF);
      m_cycle();
      reset_negative();
      break;
    }
    case 0xB: {
      DEC_R16(BC);
      m_cycle();
      break;
    }
    case 0xC: {
      INC(C);
      break;
    }
    case 0xD: {
      DEC(C);
      break;
    }
    case 0xE: {
      LD_R_R(C, read8(PC++));
      break;
    }
    case 0xF: {
      RRCA();
      break;
    }

    case 0x11: {
      LD_R16_U16(DE, read16(PC++));
      break;
    }
    case 0x12: {
      write8(DE, A);
      break;
    }
    case 0x13: {
      INC_16(DE);
      break;
    }
    case 0x14: {
      INC(D);
      break;
    }
    case 0x15: {
      DEC(D);
      break;
    }
    case 0x16: {
      LD_R_R(D, read8(PC++));
      break;
    }
    case 0x17: {
      RLA();
      break;
    }
    case 0x18: {
      i8 offset = (i8)read8(PC++);
      m_cycle();
      PC = PC + offset;
      break;
    }
    case 0x19: {
      if (((HL & 0xfff) + (DE & 0xfff)) & 0x1000) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((HL + DE) > 0xFFFF) {
        set_carry();
      } else {
        reset_carry();
      }
      HL += DE;
      m_cycle();
      reset_negative();
      break;
    }
    case 0x1A: {
      A = read8(DE);
      break;
    }
    case 0x1B: {
      DEC_R16(DE);
      m_cycle();
      break;
    }
    case 0x1C: {
      INC(E);
      break;
    }
    case 0x1D: {
      DEC(E);
      break;
    }
    case 0x1E: {
      E = read8(PC++);
      break;
    }
    case 0x1F: {
      if (get_flag(FLAG::CARRY)) {
        if (A & 0x1) {
          A >>= 1;
          A += 0x80;
          set_carry();
        } else {
          A >>= 1;
          A += 0x80;
          reset_carry();
        }
      } else {
        if (A & 0x1) {
          A >>= 1;
          set_carry();
        } else {
          A >>= 1;
          reset_carry();
        }
      }

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_zero();
      reset_negative();
      reset_half_carry();
      break;
    }
    case 0x20: {
      i8 offset = (i8)read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x21: {
      LD_R16_U16(HL, read16(PC++));

      break;
    }
    case 0x22: {
      write8(HL++, A);
      break;
    }

    case 0x23: {
      INC_16(HL);
      break;
    }
    case 0x24: {
      INC(H);
      break;
    }
    case 0x25: {
      DEC(H);
      break;
    }

    case 0x26: {
      LD_R_R(H, read8(PC++));

      break;
    }

    case 0x27: {
      // WTF is the DAA instruction?
      // https://ehaskins.com/2018-01-30%20Z80%20DAA/
      u8 adjustment = 0;
      if (get_flag(FLAG::HALF_CARRY) ||
          (!get_flag(FLAG::NEGATIVE) && (A & 0xf) > 9)) {
        adjustment |= 0x6;
      }

      if (get_flag(FLAG::CARRY) || (!get_flag(FLAG::NEGATIVE) && A > 0x99)) {
        adjustment |= 0x60;
        set_carry();
      }

      if (get_flag(FLAG::NEGATIVE)) {
        A += -adjustment;
      } else {
        A += adjustment;
      }

      A &= 0xff;

      if (A == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_half_carry();
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
    case 0x29: {
      if ((HL + HL) > 0xFFFF) {
        set_carry();
      } else {
        reset_carry();
      };

      if (((HL & 0xfff) + (HL & 0xfff)) & 0x1000) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      m_cycle();
      HL += HL;

      reset_negative();
      break;
    }
    case 0x2A: {
      A = read8(HL++);
      break;
    }
    case 0x2B: {
      DEC_R16(HL);
      m_cycle();
      break;
    }
    case 0x2D: {
      DEC(L);
      break;
    }
    case 0x2C: {
      INC(L);
      break;
    }
    case 0x2E: {
      L = read8(PC++);

      break;
    }
    case 0x2F: {
      A = A ^ 0xFF;
      set_negative();
      set_half_carry();
      break;
    }
    case 0x30: {
      i8 offset = (i8)read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x31: {
      LD_SP_U16(SP, read16(PC++));
      break;
    }
    case 0x32: {
      write8(HL, A);
      HL = HL - 1;
      break;
    }
    case 0x33: {
      m_cycle();
      SP++;
      break;
    }
    case 0x35: {
      u8 value = read8(HL);
      if (((value & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      write8(HL, --value);
      if (value == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }
    case 0x36: {
      LD_M_R(HL, read8(PC++));
      break;
    }
    case 0x37: {
      SCF();
      break;
    }
    case 0x38: {
      i8 offset = (i8)read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x39: {
      m_cycle();

      if (((HL & 0xfff) + ((SP)&0xfff)) & 0x1000) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if (HL + SP > 0xFFFF) {
        set_carry();
      } else {
        reset_carry();
      }

      HL = HL + SP;

      reset_negative();
      break;
    }
    case 0x3B: {
      DEC_SP(SP);
      break;
    }
    case 0x3C: {
      INC(A);
      break;
    }
    case 0x3D: {
      DEC(A);
      break;
    }
    case 0x3E: {
      A = read8(PC++);

      break;
    }
    case 0x3F: {
      CCF();
      break;
    }
    case 0x40: {
      LD_R_R(B, B);
      break;
    }
    case 0x41: {
      LD_R_R(B, C);
      break;
    }
    case 0x42: {
      LD_R_R(B, D);
      break;
    }
    case 0x43: {
      LD_R_R(B, E);
      break;
    }
    case 0x44: {
      LD_R_R(B, H);
      break;
    }
    case 0x45: {
      LD_R_R(B, L);
      break;
    }
    case 0x46: {
      LD_R_AMV(B, HL);
      break;
    }
    case 0x47: {
      LD_R_R(B, A);
      break;
    }
    case 0x48: {
      LD_R_R(C, B);
      break;
    }
    case 0x49: {
      break;
    }
    case 0x4A: {
      LD_R_R(C, D);
      break;
    }
    case 0x4B: {
      LD_R_R(C, E);
      break;
    }
    case 0x4C: {
      LD_R_R(C, H);
      break;
    }
    case 0x4D: {
      LD_R_R(C, L);
      break;
    }
    case 0x4E: {
      LD_R_AMV(C, HL);
      break;
    }
    case 0x4F: {
      LD_R_R(C, A);
      break;
    }
    case 0x50: {
      LD_R_R(D, B);
      break;
    }
    case 0x51: {
      LD_R_R(D, C);
      break;
    }
    case 0x53: {
      LD_R_R(D, E);
      break;
    }
    case 0x54: {
      LD_R_R(D, H);
      break;
    }
    case 0x55: {
      LD_R_R(D, L);
      break;
    }
    case 0x56: {
      LD_R_AMV(D, HL);
      break;
    }
    case 0x57: {
      LD_R_R(D, A);
      break;
    }
    case 0x58: {
      LD_R_R(E, B);
      break;
    }
    case 0x59: {
      LD_R_R(E, C);
      break;
    }
    case 0x5A: {
      LD_R_R(E, D);
      break;
    }
    case 0x5B: {
      break;
    }
    case 0x5C: {
      LD_R_R(E, H);
      break;
    }
    case 0x5D: {
      LD_R_R(E, L);
      break;
    }
    case 0x5E: {
      LD_R_AMV(E, HL);
      break;
    }

    case 0x5F: {
      LD_R_R(E, A);
      break;
    }

    case 0x60: {
      LD_R_R(H, B);
      break;
    }

    case 0x61: {
      LD_R_R(H, C);
      break;
    }

    case 0x62: {
      LD_R_R(H, D);
      break;
    }

    case 0x63: {
      LD_R_R(H, E);
      break;
    }

    case 0x65: {
      LD_R_R(H, L);
      break;
    }

    case 0x67: {
      LD_R_R(H, A);
      break;
    }
    case 0x68: {
      LD_R_R(L, B);
      break;
    }
    case 0x69: {
      LD_R_R(L, C);
      break;
    }
    case 0x6A: {
      LD_R_R(L, D);
      break;
    }
    case 0x6B: {
      LD_R_R(L, E);

      break;
    }
    case 0x6C: {
      LD_R_R(L, H);

      break;
    }
    case 0x6D: {
      break;
    }
    case 0x64: {
      break;
    }
    case 0x66: {
      LD_R_AMV(H, HL);
      break;
    }
    case 0x6E: {
      LD_R_AMV(L, HL);
      break;
    }
    case 0x6F: {
      L = A;
      break;
    }
    case 0x70: {
      LD_M_R(HL, B);
      break;
    }
    case 0x71: {
      LD_M_R(HL, C);
      break;
    }
    case 0x72: {
      LD_M_R(HL, D);
      break;
    }
    case 0x73: {
      LD_M_R(HL, E);
      break;
    }
    case 0x74: {
      LD_M_R(HL, H);
      break;
    }
    case 0x75: {
      LD_M_R(HL, L);
      break;
    }
    case 0x77: {
      LD_M_R(HL, A);
      break;
    }
    case 0x78: {
      LD_R_R(A, B);

      break;
    }
    case 0x79: {
      LD_R_R(A, C);

      break;
    }
    case 0x7A: {
      LD_R_R(A, D);

      break;
    }
    case 0x7B: {
      LD_R_R(A, E);

      break;
    }
    case 0x7C: {
      LD_R_R(A, H);

      break;
    }
    case 0x7D: {
      LD_R_R(A, L);

      break;
    }
    case 0x7E: {
      LD_R_R(A, read8(HL));
      break;
    }
    case 0x7F: {
      break;
    }
    case 0x80: {
      ADD(A, B);
      break;
    }

    case 0x81: {
      ADD(A, C);
      break;
    }

    case 0x82: {
      ADD(A, D);
      break;
    }

    case 0x83: {
      ADD(A, E);
      break;
    }

    case 0x84: {
      ADD(A, H);
      break;
    }

    case 0x85: {
      ADD(A, L);
      break;
    }

    case 0x86: {
      ADD(A, read8(HL));
      break;
    }
    case 0x87: {
      ADD(A, A);
      break;
    }

    case 0x88: {
      ADC(A, B);
      break;
    }
    case 0x89: {
      ADC(A, C);
      break;
    }
    case 0x8A: {
      ADC(A, D);
      break;
    }

    case 0x8B: {
      ADC(A, E);
      break;
    }

    case 0x8C: {
      ADC(A, H);
      break;
    }

    case 0x8D: {
      ADC(A, L);
      break;
    }

    case 0x8E: {
      ADC(A, read8(HL));
      break;
    }

    case 0x8F: {
      ADC(A, A);
      break;
    }
    case 0x90: {
      SUB(A, B);
      break;
    }
    case 0x91: {
      SUB(A, C);

      break;
    }

    case 0x92: {
      SUB(A, D);

      break;
    }

    case 0x93: {
      SUB(A, E);
      break;
    }

    case 0x94: {
      SUB(A, H);

      break;
    }

    case 0x95: {
      SUB(A, L);

      break;
    }
    case 0x96: {
      SUB(A, read8(HL));
      break;
    }
    case 0x97: {
      SUB(A, A);
      break;
    }
    case 0x98: {
      SBC(A, B);
      break;
    }
    case 0x99: {
      SBC(A, C);
      break;
    }

    case 0x9A: {
      SBC(A, D);
      break;
    }
    case 0x9B: {
      SBC(A, E);
      break;
    }
    case 0x9C: {
      SBC(A, H);
      break;
    }

    case 0x9D: {
      SBC(A, L);
      break;
    }
    case 0x9E: {
      SBC(A, read8(HL));
      break;
    }

    case 0x9F: {
      SBC(A, A);
      break;
    }

    case 0xA0: {
      AND(A, B);
      break;
    }
    case 0xA1: {
      AND(A, C);

      break;
    }
    case 0xA2: {
      AND(A, D);

      break;
    }
    case 0xA3: {
      AND(A, E);

      break;
    }
    case 0xA4: {
      AND(A, H);

      break;
    }
    case 0xA5: {
      AND(A, L);

      break;
    }
    case 0xA6: {
      AND(A, read8(HL));
      break;
    }
    case 0xA7: {
      AND(A, A);
      break;
    }

    case 0xA8: {
      XOR(A, B);
      break;
    }
    case 0xA9: {
      XOR(A, C);
      break;
    }
    case 0xAA: {
      XOR(A, D);
      break;
    }
    case 0xAB: {
      XOR(A, E);
      break;
    }
    case 0xAC: {
      XOR(A, H);
      break;
    }
    case 0xAD: {
      XOR(A, L);
      break;
    }
    case 0xAE: {
      XOR(A, read8(HL));
      break;
    }
    case 0xAF: {
      XOR(A, A);
      break;
    }

    case 0xB0: {
      OR(A, B);
      break;
    }
    case 0xB1: {
      OR(A, C);
      break;
    }
    case 0xB2: {
      OR(A, D);
      break;
    }
    case 0xB3: {
      OR(A, E);
      break;
    }
    case 0xB4: {
      OR(A, H);
      break;
    }
    case 0xB5: {
      OR(A, L);
      break;
    }
    case 0xB6: {
      OR(A, read8(HL));
      break;
    }

    case 0xB7: {
      OR(A, A);
      break;
    }
    case 0xB8: {
      CP(A, B);
      break;
    }

    case 0xB9: {
      CP(A, C);
      break;
    }
    case 0xBA: {
      CP(A, D);
      break;
    }

    case 0xBB: {
      CP(A, E);
      break;
    }
    case 0xBC: {
      CP(A, H);
      break;
    }
    case 0xBD: {
      CP(A, L);
      break;
    }

    case 0xBE: {
      CP(A, read8(HL));
      break;
    }

    case 0xBF: {
      CP(A, A);
      break;
    }

    case 0xC0: {
      m_cycle();
      if (!get_flag(FLAG::ZERO)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC1: {
      POP(BC);
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
      PUSH(BC);
      break;
    }
    case 0xC6: {
      ADD(A, read8(PC++));
      break;
    }
    case 0xC7: {
      RST(0);
      break;
    }
    case 0xC8: {
      m_cycle();
      if (get_flag(FLAG::ZERO)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC9: {
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      m_cycle();
      PC = (high << 8) + low;
      break;
    }
    case 0xCA: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xCB: {
      switch (read8(PC++)) {
        case 0x0: {
          if (B & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          B <<= 1;
          B |= get_flag(FLAG::CARRY);
          if (B == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x1: {
          if (C & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          C <<= 1;
          C |= get_flag(FLAG::CARRY);
          if (C == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x2: {
          if (D & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          D <<= 1;
          D |= get_flag(FLAG::CARRY);
          if (D == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x3: {
          if (E & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          E <<= 1;
          E |= get_flag(FLAG::CARRY);
          if (E == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x4: {
          if (H & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          H <<= 1;
          H |= get_flag(FLAG::CARRY);
          if (H == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x5: {
          if (L & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          L <<= 1;
          L |= get_flag(FLAG::CARRY);
          if (L == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x6: {
          u8 _hl = read8(HL);

          if (_hl & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          _hl <<= 1;
          _hl |= get_flag(FLAG::CARRY);
          write8(HL, _hl);
          if (_hl == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x7: {
          if (A & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          A <<= 1;
          A |= get_flag(FLAG::CARRY);
          if (A == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x8: {
          if (B & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          B >>= 1;
          B |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (B == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x9: {
          if (C & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          C >>= 1;
          C |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (C == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0xA: {
          if (D & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          D >>= 1;
          D |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (D == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0xB: {
          if (E & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          E >>= 1;
          E |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (E == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0xC: {
          if (H & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          H >>= 1;
          H |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (H == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0xD: {
          if (L & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          L >>= 1;
          L |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (L == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        // case 0xE: {
        //   u8 _hl = read8(HL);
        //   if (_hl & 0x1) {
        //     set_carry();
        //   } else {
        //     reset_carry();
        //   }
        //   _hl >>= 1;
        //   _hl |= get_flag(FLAG::CARRY) ? 0x80 : 0;
        //   write8(HL, _hl);
        //   if (_hl == 0) {
        //     set_zero();
        //   } else {
        //     reset_zero();
        //   }

        //   reset_negative();
        //   reset_half_carry();
        //   break;
        // }
        case 0xF: {
          if (A & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          A >>= 1;
          A |= get_flag(FLAG::CARRY) ? 0x80 : 0;
          if (A == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x19: {
          if (get_flag(FLAG::CARRY)) {
            if (C & 0x1) {
              C >>= 1;
              C += 0x80;
              set_carry();
            } else {
              C >>= 1;
              C += 0x80;
              reset_carry();
            }
          } else {
            if (C & 0x1) {
              C >>= 1;
              set_carry();
            } else {
              C >>= 1;
              reset_carry();
            }
          }

          if (C == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        case 0x1A: {
          if (get_flag(FLAG::CARRY)) {
            if (D & 0x1) {
              D >>= 1;
              D += 0x80;
              set_carry();
            } else {
              D >>= 1;
              D += 0x80;
              reset_carry();
            }
          } else {
            if (D & 0x1) {
              D >>= 1;
              set_carry();
            } else {
              D >>= 1;
              reset_carry();
            }
          }

          if (D == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x1B: {
          if (get_flag(FLAG::CARRY)) {
            if (E & 0x1) {
              E >>= 1;
              E += 0x80;
              set_carry();
            } else {
              E >>= 1;
              E += 0x80;
              reset_carry();
            }
          } else {
            if (E & 0x1) {
              E >>= 1;
              set_carry();
            } else {
              E >>= 1;
              reset_carry();
            }
          }

          if (E == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x30: {
          u8 hi = B & 0xF0;
          u8 lo = B & 0xF;

          B = (lo << 8) + hi;

          if (B == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        case 0x31: {
          u8 hi = C & 0xF0;
          u8 lo = C & 0xF;

          C = (lo << 8) + hi;

          if (C == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        case 0x32: {
          u8 hi = D & 0xF0;
          u8 lo = D & 0xF;

          D = (lo << 8) + hi;

          if (D == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        case 0x33: {
          u8 hi = E & 0xF0;
          u8 lo = E & 0xF;

          E = (lo << 8) + hi;

          if (E == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        case 0x34: {
          u8 hi = H & 0xF0;
          u8 lo = H & 0xF;

          H = (lo << 8) + hi;

          if (H == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        case 0x35: {
          u8 hi = L & 0xF0;
          u8 lo = L & 0xF;

          L = (lo << 8) + hi;

          if (L == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }
        // case 0x36: {
        //   u8 val = read8(HL);
        //   u8 hi  = val & 0xF0;
        //   u8 lo  = val & 0xF;

        //   val = (lo << 8) + hi;

        //   write8(HL, val);

        //   if (val == 0) {
        //     set_zero();
        //   } else {
        //     reset_zero();
        //   }
        //   reset_carry();
        //   reset_half_carry();
        //   reset_negative();

        //   break;
        // }
        case 0x37: {
          u8 hi = A & 0xF0;
          u8 lo = A & 0xF;

          A = (lo << 8) + hi;

          if (A == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_carry();
          reset_half_carry();
          reset_negative();

          break;
        }

        case 0x38: {
          if (B & 0x1) {
            set_carry();
          } else {
            reset_carry();
          }
          B >>= 1;

          if (B == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x39: {
          if (C & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          C >>= 1;

          if (C == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3A: {
          if (D & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          D >>= 1;

          if (D == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3B: {
          if (E & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          E >>= 1;

          if (E == 0) {
            set_zero();
          } else {
            reset_zero();
          }
          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3C: {
          if (H & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          H >>= 1;
          if (H == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        case 0x3D: {
          if (L & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          L >>= 1;
          if (L == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }
        // case 0x3E: {
        //   u8 val = read8(HL);
        //   if (val & 0x80) {
        //     set_carry();
        //   } else {
        //     reset_carry();
        //   }
        //   val >>= 1;
        //   write8(HL, val);
        //   H = (HL & 0xFF00) >> 8;
        //   L = (HL & 0xFF);
        //   reset_negative();
        //   reset_half_carry();
        //   break;
        // }
        case 0x3F: {
          if (A & 0x80) {
            set_carry();
          } else {
            reset_carry();
          }
          A >>= 1;

          if (A == 0) {
            set_zero();
          } else {
            reset_zero();
          }

          reset_negative();
          reset_half_carry();
          break;
        }

        default: {
          fmt::println("[CPU] unimplemented CB op: {:#04x}", peek(PC - 1));
          exit(-1);
        }
      }
      break;
    }
    case 0xCC: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        m_cycle();
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
    case 0xCF: {
      RST(0x8);
      break;
    }
    case 0xCE: {
      ADC(A, read8(PC++));
      break;
    }
    case 0xD0: {
      m_cycle();
      if (!get_flag(FLAG::CARRY)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD1: {
      POP(DE);
      break;
    }
    case 0xD2: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD4: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        m_cycle();
      }
      break;
    }
    case 0xD5: {
      PUSH(DE);
      break;
    }
    case 0xD6: {
      u8 val = read8(PC++);
      if (((A & 0xf) - ((val)&0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((A - val) < 0) {
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

      set_negative();
      break;
    }
    case 0xD7: {
      RST(0x10);
      break;
    }
    case 0xD8: {
      m_cycle();
      if (get_flag(FLAG::CARRY)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD9: {
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      m_cycle();
      IME = true;
      PC  = (high << 8) + low;
      break;
    }
    case 0xDA: {
      m_cycle();
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xDC: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        m_cycle();
      }
      break;
    }
    case 0xDF: {
      RST(0x18);
      break;
    }
    case 0xDE: {
      SBC(A, read8(PC++));
      break;
    }
    case 0xE0: {
      LD_M_R(0xFF00 + read8(PC++), A);
      break;
    }

    case 0xE1: {
      POP(HL);
      break;
    }
    case 0xE2: {
      LD_M_R(0xFF00 + C, A);
      break;
    }
    case 0xE5: {
      PUSH(HL);
      break;
    }
    case 0xE6: {
      AND(A, read8(PC++));
      break;
    }
    case 0xE7: {
      RST(0x20);
      break;
    }
    case 0xE8: {
      ADD_SP_E8();
      break;
    }
    case 0xE9: {
      PC = HL;
      break;
    }
    case 0xEA: {
      LD_M_R(read16(PC++), A);
      break;
    }
    case 0xEE: {
      XOR(A, read8(PC++));
      break;
    }
    case 0xEF: {
      RST(0x28);
      break;
    }
    case 0xF0: {
      LD_R_R(A, read8(0xFF00 + read8(PC++)));
      break;
    }
    case 0xF1: {
      POP(AF);
      break;
    }
    case 0xF2: {
      LD_R_R(A, read8(0xFF00 + C));
      break;
    }
    case 0xF3: {
      IME = false;
      break;
    }
    case 0xF5: {
      PUSH(AF);
      break;
    }
    case 0xF6: {
      OR(A, read8(PC++));
      break;
    }
    case 0xF7: {
      RST(0x30);
      break;
    }
    case 0xF8: {
      LD_HL_SP_E8();
      break;
    }
    case 0xF9: {
      SP = HL;
      m_cycle();
      break;
    }
    case 0xFA: {
      LD_R_R(A, read8(read16(PC++)));
      break;
    }

    case 0xFB: {
      IME = true;
      break;
    }
    case 0xFE: {
      CP(A, read8(PC++));
      break;
    }
    case 0xFF: {
      RST(0x38);
      break;
    }
    default: {
      fmt::println("[CPU] unimplemented opcode: {:#04x}", opcode);
      exit(0);
      throw std::runtime_error(
          fmt::format("[CPU] unimplemented opcode: {:#04x}", opcode));
    }
  }
  handle_interrupts();
}