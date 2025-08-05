#include "cpu.hpp"

#include <bitset>
#include <cassert>
#include "bus.hpp"
#include "common.hpp"
#include "instructions.hpp"
#include "ppu.hpp"

void SM83::set_flag(FLAG flag) { F |= (1 << (u8)flag); };

void SM83::unset_flag(FLAG flag) { F &= ~(1 << (u8)flag); };

u8 SM83::get_flag(FLAG flag) const { return F & (1 << (u8)flag) ? 1 : 0; }

void SM83::handle_interrupts() {
  // cpu_logger->info("IF:   {:08b}", bus->io[IF]);
  // cpu_logger->info("IE:   {:08b}", bus->io[IE]);
  if (IME && bus->io[IE] && bus->io[IF]) {
    // ("[CPU] IME:  {:08b}", IME);

    std::bitset<8> ie_set(bus->io[IE]);
    std::bitset<8> if_set(bus->io[IF]);

    for (size_t i = 0; i < 5; i++) {
      if (if_set[i] && ie_set[i]) {
        IME = false;

        // ("[CPU] handling interrupt: {:#04x}", i);
        // ("[CPU] PC: {:#04x}", PC);
        if_set[i].flip();
        // bus->io[IF] &= ~(1 << i);
        bus->m_cycle();
        bus->m_cycle();
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));
        PC = INTERRUPT_TABLE[i];
        bus->m_cycle();
        // ("[CPU] AFTER PC: {:#04x}", PC);
      };
    }

    bus->io[IE] = ie_set.to_ulong();
    bus->io[IF] = if_set.to_ulong();
    // Bit 0 (VBlank) has the highest priority, and Bit 4 (Joypad) has the
    // lowest priority.
  }
};
void SM83::step() {
#ifndef CPU_TEST_MODE

  if (status == STATUS::PAUSED) { return; }
  handle_interrupts();
  // (
  //     "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} "
  //     "L: {:02X} SP: {:04X} PC: {:02X}:{:04X} ({:02X} {:02X} {:02X} {:02X}) "
  //     "TE: {}",
  //     A, F, B, C, D, E, H, L, SP, mapper->rom_bank, PC, bus->peek(PC), bus->peek(PC + 1),
  //     bus->peek(PC + 2), bus->peek(PC + 3), timer->ticking_enabled);
#endif

  u8 opcode = bus->read8(PC++);
  switch (opcode) {
    case 0x0: {
      Instructions::NOP(this);
      break;
    }
    case 0x1: {
      Instructions::LD_R16_U16(this, BC, bus->read16(PC++));
      PC++;
      break;
    }
    case 0x2: {
      Instructions::LD_M_R(this, BC, A);
      break;
    }
    case 0x3: {
      Instructions::INC_16(this, BC);
      break;
    }
    case 0x4: {
      Instructions::INC(this, B);
      break;
    }
    case 0x5: {
      Instructions::DEC(this, B);
      break;
    }
    case 0x6: {
      Instructions::LD_R_R(B, bus->read8(PC++));
      break;
    }
    case 0x7: {
      Instructions::RLCA(this);
      break;
    }
    case 0x8: {
      Instructions::LD_U16_SP(this, bus->read16(PC++), SP);
      PC++;
      break;
    }
    case 0x9: {
      Instructions::ADD_HL_BC(this);
      break;
    }
    case 0xA: {
      A = bus->read8(BC);
      break;
    }
    case 0xB: {
      
      Instructions::DEC_R16(this, BC);
      bus->m_cycle();
      break;
    }
    case 0xC: {
      Instructions::INC(this, C);
      break;
    }
    case 0xD: {
      Instructions::DEC(this, C);
      break;
    }
    case 0xE: {
      Instructions::LD_R_R(C, bus->read8(PC++));
      break;
    }
    case 0xF: {
      Instructions::RRCA(this);
      break;
    }
    case 0x10: {
      Instructions::STOP();
      PC++;
      break;
    }
    case 0x11: {
      Instructions::LD_R16_U16(this, DE, bus->read16(PC++));
      PC++;
      break;
    }
    case 0x12: {
      bus->write8(DE, A);
      break;
    }
    case 0x13: {
      Instructions::INC_16(this, DE);
      break;
    }
    case 0x14: {
      Instructions::INC(this, D);
      break;
    }
    case 0x15: {
      Instructions::DEC(this, D);
      break;
    }
    case 0x16: {
      Instructions::LD_R_R(D, bus->read8(PC++));
      break;
    }
    case 0x17: {
      Instructions::RLA(this);
      break;
    }
    case 0x18: {
      i8 offset = bus->read8(PC++);
      bus->m_cycle();
      PC = PC + offset;
      break;
    }
    case 0x19: {
      Instructions::ADD_HL_DE(this);
      break;
    }
    case 0x1A: {
      A = bus->read8(DE);
      break;
    }
    case 0x1B: {
      Instructions::DEC_R16(this, DE);
      bus->m_cycle();
      break;
    }
    case 0x1C: {
      Instructions::INC(this, E);
      break;
    }
    case 0x1D: {
      Instructions::DEC(this, E);
      break;
    }
    case 0x1E: {
      E = bus->read8(PC++);
      break;
    }
    case 0x1F: {
      Instructions::RRA(this);
      break;
    }
    case 0x20: {
      i8 offset = (i8)bus->read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        bus->m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x21: {
      Instructions::LD_R16_U16(this, HL, bus->read16(PC++));
PC++;
      break;
    }
    case 0x22: {
      bus->write8(HL++, A);
      break;
    }

    case 0x23: {
      Instructions::INC_16(this, HL);
      break;
    }
    case 0x24: {
      Instructions::INC(this, H);
      break;
    }
    case 0x25: {
      Instructions::DEC(this, H);
      break;
    }

    case 0x26: {
      Instructions::LD_R_R(H, bus->read8(PC++));

      break;
    }

    case 0x27: {
      Instructions::DAA(this);
      break;
    }

    case 0x28: {
      i8 offset = (i8)bus->read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        bus->m_cycle();
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
      // ("2");
      if (((HL & 0xfff) + (HL & 0xfff)) & 0x1000) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      bus->m_cycle();
      HL += HL;

      reset_negative();
      break;
    }
    case 0x2A: {
      A = bus->read8(HL++);
      break;
    }
    case 0x2B: {
      Instructions::DEC_R16(this, HL);
      bus->m_cycle();
      break;
    }
    case 0x2D: {
      Instructions::DEC(this, L);
      break;
    }
    case 0x2C: {
      Instructions::INC(this, L);
      break;
    }
    case 0x2E: {
      L = bus->read8(PC++);

      break;
    }
    case 0x2F: {
      A = A ^ 0xFF;
      set_negative();
      set_half_carry();
      break;
    }
    case 0x30: {
      i8 offset = (i8)bus->read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        bus->m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x31: {
      Instructions::LD_SP_U16(this, bus->read16(PC++));
      PC++;
      break;
    }
    case 0x32: {
      bus->write8(HL, A);
      HL = HL - 1;
      break;
    }
    case 0x33: {
      bus->m_cycle();
      SP++;
      break;
    }
    case 0x34: {
      u8 hl = bus->read8(HL);
      Instructions::INC(this, hl);
      bus->write8(HL, hl);
      break;
    }
    case 0x35: {
      u8 value = bus->read8(HL);
      if (((value & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      bus->write8(HL, --value);
      if (value == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
      break;
    }
    case 0x36: {
      Instructions::LD_M_R(this, HL, bus->read8(PC++));
      break;
    }
    case 0x37: {
      Instructions::SCF(this);
      break;
    }
    case 0x38: {
      i8 offset = (i8)bus->read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        bus->m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x39: {
      bus->m_cycle();

      if (((HL & 0xfff) + ((SP) & 0xfff)) & 0x1000) {
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
    case 0x3A: {
      A = bus->read8(HL--);
      break;
    }
    case 0x3B: {
      Instructions::DEC_SP(this);
      break;
    }
    case 0x3C: {
      Instructions::INC(this, A);
      break;
    }
    case 0x3D: {
      Instructions::DEC(this, A);
      break;
    }
    case 0x3E: {
      A = bus->read8(PC++);

      break;
    }
    case 0x3F: {
      Instructions::CCF(this);
      break;
    }
    case 0x40: {
      Instructions::LD_R_R(B, B);
      break;
    }
    case 0x41: {
      Instructions::LD_R_R(B, C);
      break;
    }
    case 0x42: {
      Instructions::LD_R_R(B, D);
      break;
    }
    case 0x43: {
      Instructions::LD_R_R(B, E);
      break;
    }
    case 0x44: {
      Instructions::LD_R_R(B, H);
      break;
    }
    case 0x45: {
      Instructions::LD_R_R(B, L);
      break;
    }
    case 0x46: {
      Instructions::LD_R_AMV(this, B, HL);
      break;
    }
    case 0x47: {
      Instructions::LD_R_R(B, A);
      break;
    }
    case 0x48: {
      Instructions::LD_R_R(C, B);
      break;
    }
    case 0x49: {
      break;
    }
    case 0x4A: {
      Instructions::LD_R_R(C, D);
      break;
    }
    case 0x4B: {
      Instructions::LD_R_R(C, E);
      break;
    }
    case 0x4C: {
      Instructions::LD_R_R(C, H);
      break;
    }
    case 0x4D: {
      Instructions::LD_R_R(C, L);
      break;
    }
    case 0x4E: {
      Instructions::LD_R_AMV(this, C, HL);
      break;
    }
    case 0x4F: {
      Instructions::LD_R_R(C, A);
      break;
    }
    case 0x50: {
      Instructions::LD_R_R(D, B);
      break;
    }
    case 0x51: {
      Instructions::LD_R_R(D, C);
      break;
    }
    case 0x52: {
      // Instructions::LD_R_R(D, D);
      break;
    }

    case 0x53: {
      Instructions::LD_R_R(D, E);
      break;
    }
    case 0x54: {
      Instructions::LD_R_R(D, H);
      break;
    }
    case 0x55: {
      Instructions::LD_R_R(D, L);
      break;
    }
    case 0x56: {
      Instructions::LD_R_AMV(this, D, HL);
      break;
    }
    case 0x57: {
      Instructions::LD_R_R(D, A);
      break;
    }
    case 0x58: {
      Instructions::LD_R_R(E, B);
      break;
    }
    case 0x59: {
      Instructions::LD_R_R(E, C);
      break;
    }
    case 0x5A: {
      Instructions::LD_R_R(E, D);
      break;
    }
    case 0x5B: {
      break;
    }
    case 0x5C: {
      Instructions::LD_R_R(E, H);
      break;
    }
    case 0x5D: {
      Instructions::LD_R_R(E, L);
      break;
    }
    case 0x5E: {
      Instructions::LD_R_AMV(this, E, HL);
      break;
    }

    case 0x5F: {
      Instructions::LD_R_R(E, A);
      break;
    }

    case 0x60: {
      Instructions::LD_R_R(H, B);
      break;
    }

    case 0x61: {
      Instructions::LD_R_R(H, C);
      break;
    }

    case 0x62: {
      Instructions::LD_R_R(H, D);
      break;
    }

    case 0x63: {
      Instructions::LD_R_R(H, E);
      break;
    }

    case 0x65: {
      Instructions::LD_R_R(H, L);
      break;
    }

    case 0x67: {
      Instructions::LD_R_R(H, A);
      break;
    }
    case 0x68: {
      Instructions::LD_R_R(L, B);
      break;
    }
    case 0x69: {
      Instructions::LD_R_R(L, C);
      break;
    }
    case 0x6A: {
      Instructions::LD_R_R(L, D);
      break;
    }
    case 0x6B: {
      Instructions::LD_R_R(L, E);

      break;
    }
    case 0x6C: {
      Instructions::LD_R_R(L, H);

      break;
    }
    case 0x6D: {
      break;
    }
    case 0x64: {
      break;
    }
    case 0x66: {
      Instructions::LD_R_AMV(this, H, HL);
      break;
    }
    case 0x6E: {
      Instructions::LD_R_AMV(this, L, HL);
      break;
    }
    case 0x6F: {
      L = A;
      break;
    }
    case 0x70: {
      Instructions::LD_M_R(this, HL, B);
      break;
    }
    case 0x71: {
      Instructions::LD_M_R(this, HL, C);
      break;
    }
    case 0x72: {
      Instructions::LD_M_R(this, HL, D);
      break;
    }
    case 0x73: {
      Instructions::LD_M_R(this, HL, E);
      break;
    }
    case 0x74: {
      Instructions::LD_M_R(this, HL, H);
      break;
    }
    case 0x75: {
      Instructions::LD_M_R(this, HL, L);
      break;
    }
    case 0x76: {
      Instructions::HALT(this);
      break;
    }
    case 0x77: {
      Instructions::LD_M_R(this, HL, A);
      break;
    }
    case 0x78: {
      Instructions::LD_R_R(A, B);

      break;
    }
    case 0x79: {
      Instructions::LD_R_R(A, C);

      break;
    }
    case 0x7A: {
      Instructions::LD_R_R(A, D);

      break;
    }
    case 0x7B: {
      Instructions::LD_R_R(A, E);

      break;
    }
    case 0x7C: {
      Instructions::LD_R_R(A, H);

      break;
    }
    case 0x7D: {
      Instructions::LD_R_R(A, L);

      break;
    }
    case 0x7E: {
      Instructions::LD_R_R(A, bus->read8(HL));
      break;
    }
    case 0x7F: {
      break;
    }
    case 0x80: {
      Instructions::ADD(this, A, B);
      break;
    }

    case 0x81: {
      Instructions::ADD(this, A, C);
      break;
    }

    case 0x82: {
      Instructions::ADD(this, A, D);
      break;
    }

    case 0x83: {
      Instructions::ADD(this, A, E);
      break;
    }

    case 0x84: {
      Instructions::ADD(this, A, H);
      break;
    }

    case 0x85: {
      Instructions::ADD(this, A, L);
      break;
    }

    case 0x86: {
      Instructions::ADD(this, A, bus->read8(HL));
      break;
    }
    case 0x87: {
      Instructions::ADD(this, A, A);
      break;
    }

    case 0x88: {
      Instructions::ADC(this, A, B);
      break;
    }
    case 0x89: {
      Instructions::ADC(this, A, C);
      break;
    }
    case 0x8A: {
      Instructions::ADC(this, A, D);
      break;
    }

    case 0x8B: {
      Instructions::ADC(this, A, E);
      break;
    }

    case 0x8C: {
      Instructions::ADC(this, A, H);
      break;
    }

    case 0x8D: {
      Instructions::ADC(this, A, L);
      break;
    }

    case 0x8E: {
      Instructions::ADC(this, A, bus->read8(HL));
      break;
    }

    case 0x8F: {
      Instructions::ADC(this, A, A);
      break;
    }
    case 0x90: {
      Instructions::SUB(this, A, B);
      break;
    }
    case 0x91: {
      Instructions::SUB(this, A, C);

      break;
    }

    case 0x92: {
      Instructions::SUB(this, A, D);

      break;
    }

    case 0x93: {
      Instructions::SUB(this, A, E);
      break;
    }

    case 0x94: {
      Instructions::SUB(this, A, H);

      break;
    }

    case 0x95: {
      Instructions::SUB(this, A, L);

      break;
    }
    case 0x96: {
      Instructions::SUB(this, A, bus->read8(HL));
      break;
    }
    case 0x97: {
      Instructions::SUB(this, A, A);
      break;
    }
    case 0x98: {
      Instructions::SBC(this, A, B);
      break;
    }
    case 0x99: {
      Instructions::SBC(this, A, C);
      break;
    }

    case 0x9A: {
      Instructions::SBC(this, A, D);
      break;
    }
    case 0x9B: {
      Instructions::SBC(this, A, E);
      break;
    }
    case 0x9C: {
      Instructions::SBC(this, A, H);
      break;
    }

    case 0x9D: {
      Instructions::SBC(this, A, L);
      break;
    }
    case 0x9E: {
      Instructions::SBC(this, A, bus->read8(HL));
      break;
    }

    case 0x9F: {
      Instructions::SBC(this, A, A);
      break;
    }

    case 0xA0: {
      Instructions::AND(this, A, B);
      break;
    }
    case 0xA1: {
      Instructions::AND(this, A, C);

      break;
    }
    case 0xA2: {
      Instructions::AND(this, A, D);

      break;
    }
    case 0xA3: {
      Instructions::AND(this, A, E);

      break;
    }
    case 0xA4: {
      Instructions::AND(this, A, H);

      break;
    }
    case 0xA5: {
      Instructions::AND(this, A, L);

      break;
    }
    case 0xA6: {
      Instructions::AND(this, A, bus->read8(HL));
      break;
    }
    case 0xA7: {
      Instructions::AND(this, A, A);
      break;
    }

    case 0xA8: {
      Instructions::XOR(this, A, B);
      break;
    }
    case 0xA9: {
      Instructions::XOR(this, A, C);
      break;
    }
    case 0xAA: {
      Instructions::XOR(this, A, D);
      break;
    }
    case 0xAB: {
      Instructions::XOR(this, A, E);
      break;
    }
    case 0xAC: {
      Instructions::XOR(this, A, H);
      break;
    }
    case 0xAD: {
      Instructions::XOR(this, A, L);
      break;
    }
    case 0xAE: {
      Instructions::XOR(this, A, bus->read8(HL));
      break;
    }
    case 0xAF: {
      Instructions::XOR(this, A, A);
      break;
    }

    case 0xB0: {
      Instructions::OR(this, A, B);
      break;
    }
    case 0xB1: {
      Instructions::OR(this, A, C);
      break;
    }
    case 0xB2: {
      Instructions::OR(this, A, D);
      break;
    }
    case 0xB3: {
      Instructions::OR(this, A, E);
      break;
    }
    case 0xB4: {
      Instructions::OR(this, A, H);
      break;
    }
    case 0xB5: {
      Instructions::OR(this, A, L);
      break;
    }
    case 0xB6: {
      Instructions::OR(this, A, bus->read8(HL));
      break;
    }

    case 0xB7: {
      Instructions::OR(this, A, A);
      break;
    }
    case 0xB8: {
      Instructions::CP(this, A, B);
      break;
    }

    case 0xB9: {
      Instructions::CP(this, A, C);
      break;
    }
    case 0xBA: {
      Instructions::CP(this, A, D);
      break;
    }

    case 0xBB: {
      Instructions::CP(this, A, E);
      break;
    }
    case 0xBC: {
      Instructions::CP(this, A, H);
      break;
    }
    case 0xBD: {
      Instructions::CP(this, A, L);
      break;
    }

    case 0xBE: {
      Instructions::CP(this, A, bus->read8(HL));
      break;
    }

    case 0xBF: {
      Instructions::CP(this, A, A);
      break;
    }

    case 0xC0: {
      bus->m_cycle();
      if (!get_flag(FLAG::ZERO)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC1: {
      Instructions::POP(this, BC);
      break;
    }
    case 0xC2: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC3: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);

      PC = (high << 8) + low;
      bus->m_cycle();
      break;
    }
    case 0xC4: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (!get_flag(FLAG::ZERO)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        bus->m_cycle();
      }
      break;
    }
    case 0xC5: {
      Instructions::PUSH(this, BC);
      break;
    }
    case 0xC6: {
      Instructions::ADD(this, A, bus->read8(PC++));
      break;
    }
    case 0xC7: {
      Instructions::RST(this, 0);
      break;
    }
    case 0xC8: {
      bus->m_cycle();
      if (get_flag(FLAG::ZERO)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xC9: {
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      bus->m_cycle();
      PC = (high << 8) + low;
      break;
    }
    case 0xCA: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xCB: {
      switch (bus->read8(PC++)) {
        case 0x0: {
          Instructions::RLC(this, B);
          break;
        }

        case 0x1: {
          Instructions::RLC(this, C);
          break;
        }

        case 0x2: {
          Instructions::RLC(this, D);
          break;
        }

        case 0x3: {
          Instructions::RLC(this, E);
          break;
        }

        case 0x4: {
          Instructions::RLC(this, H);
          break;
        }

        case 0x5: {
          Instructions::RLC(this, L);
          break;
        }

        case 0x6: {
          Instructions::RLC_HL(this);
          break;
        }

        case 0x7: {
          Instructions::RLC(this, A);
          break;
        }

        case 0x8: {
          Instructions::RRC(this, B);
          break;
        }
        case 0x9: {
          Instructions::RRC(this, C);
          break;
        }
        case 0xA: {
          Instructions::RRC(this, D);
          break;
        }
        case 0xB: {
          Instructions::RRC(this, E);
          break;
        }
        case 0xC: {
          Instructions::RRC(this, H);
          break;
        }
        case 0xD: {
          Instructions::RRC(this, L);
          break;
        }
        case 0xE: {
          Instructions::RRC_HL(this);
          break;
        }
        case 0xF: {
          Instructions::RRC(this, A);
          break;
        }

        case 0x10: {
          Instructions::RL(this, B);
          break;
        }
        case 0x11: {
          Instructions::RL(this, C);
          break;
        }
        case 0x12: {
          Instructions::RL(this, D);
          break;
        }
        case 0x13: {
          Instructions::RL(this, E);
          break;
        }
        case 0x14: {
          Instructions::RL(this, H);
          break;
        }
        case 0x15: {
          Instructions::RL(this, L);
          break;
        }
        case 0x16: {
          Instructions::RL_HL(this);
          break;
        }
        case 0x17: {
          Instructions::RL(this, A);
          break;
        }

        case 0x18: {
          Instructions::RR(this, B);
          break;
        }
        case 0x19: {
          Instructions::RR(this, C);
          break;
        }
        case 0x1A: {
          Instructions::RR(this, D);
          break;
        }
        case 0x1B: {
          Instructions::RR(this, E);
          break;
        }
        case 0x1C: {
          Instructions::RR(this, H);
          break;
        }
        case 0x1D: {
          Instructions::RR(this, L);
          break;
        }
        case 0x1E: {
          Instructions::RR_HL(this);
          break;
        }
        case 0x1F: {
          Instructions::RR(this, A);
          break;
        }

        case 0x20: {
          Instructions::SLA(this, B);
          break;
        }
        case 0x21: {
          Instructions::SLA(this, C);
          break;
        }
        case 0x22: {
          Instructions::SLA(this, D);
          break;
        }
        case 0x23: {
          Instructions::SLA(this, E);
          break;
        }
        case 0x24: {
          Instructions::SLA(this, H);
          break;
        }
        case 0x25: {
          Instructions::SLA(this, L);
          break;
        }
        case 0x26: {
          Instructions::SLA_HL(this);
          break;
        }
        case 0x27: {
          Instructions::SLA(this, A);
          break;
        }
        case 0x28: {
          Instructions::SRA(this, B);
          break;
        }
        case 0x29: {
          Instructions::SRA(this, C);
          break;
        }
        case 0x2A: {
          Instructions::SRA(this, D);
          break;
        }
        case 0x2B: {
          Instructions::SRA(this, E);
          break;
        }
        case 0x2C: {
          Instructions::SRA(this, H);
          break;
        }
        case 0x2D: {
          Instructions::SRA(this, L);
          break;
        }
        case 0x2E: {
          Instructions::SRA_HL(this);
          break;
        }
        case 0x2F: {
          Instructions::SRA(this, A);
          break;
        }
        case 0x30: {
          Instructions::SWAP(this, B);
          break;
        }
        case 0x31: {
          Instructions::SWAP(this, C);
          break;
        }
        case 0x32: {
          Instructions::SWAP(this, D);
          break;
        }
        case 0x33: {
          Instructions::SWAP(this, E);
          break;
        }
        case 0x34: {
          Instructions::SWAP(this, H);
          break;
        }
        case 0x35: {
          Instructions::SWAP(this, L);
          break;
        }
        case 0x36: {
          Instructions::SWAP_HL(this);
          break;
        }
        case 0x37: {
          Instructions::SWAP(this, A);
          break;
        }

        case 0x38: {
          Instructions::SRL(this, B);
          break;
        }

        case 0x39: {
          Instructions::SRL(this, C);
          break;
        }

        case 0x3A: {
          Instructions::SRL(this, D);
          break;
        }

        case 0x3B: {
          Instructions::SRL(this, E);
          break;
        }

        case 0x3C: {
          Instructions::SRL(this, H);
          break;
        }

        case 0x3D: {
          Instructions::SRL(this, L);
          break;
        }

        case 0x3E: {
          Instructions::SRL_HL(this);
          break;
        }

        case 0x3F: {
          Instructions::SRL(this, A);
          break;
        }

        case 0x40: {
          Instructions::BIT(this, 0, B);
          break;
        }
        case 0x41: {
          Instructions::BIT(this, 0, C);
          break;
        }
        case 0x42: {
          Instructions::BIT(this, 0, D);
          break;
        }
        case 0x43: {
          Instructions::BIT(this, 0, E);
          break;
        }
        case 0x44: {
          Instructions::BIT(this, 0, H);
          break;
        }
        case 0x45: {
          Instructions::BIT(this, 0, L);
          break;
        }
        case 0x46: {
          Instructions::BIT(this, 0, bus->read8(HL));
          break;
        }
        case 0x47: {
          Instructions::BIT(this, 0, A);
          break;
        }

        case 0x48: {
          Instructions::BIT(this, 1, B);
          break;
        }
        case 0x49: {
          Instructions::BIT(this, 1, C);
          break;
        }
        case 0x4A: {
          Instructions::BIT(this, 1, D);
          break;
        }
        case 0x4B: {
          Instructions::BIT(this, 1, E);
          break;
        }
        case 0x4C: {
          Instructions::BIT(this, 1, H);
          break;
        }
        case 0x4D: {
          Instructions::BIT(this, 1, L);
          break;
        }
        case 0x4E: {
          Instructions::BIT(this, 1, bus->read8(HL));
          break;
        }
        case 0x4F: {
          Instructions::BIT(this, 1, A);
          break;
        }

        case 0x50: {
          Instructions::BIT(this, 2, B);
          break;
        }
        case 0x51: {
          Instructions::BIT(this, 2, C);
          break;
        }
        case 0x52: {
          Instructions::BIT(this, 2, D);
          break;
        }
        case 0x53: {
          Instructions::BIT(this, 2, E);
          break;
        }
        case 0x54: {
          Instructions::BIT(this, 2, H);
          break;
        }
        case 0x55: {
          Instructions::BIT(this, 2, L);
          break;
        }
        case 0x56: {
          Instructions::BIT(this, 2, bus->read8(HL));
          break;
        }
        case 0x57: {
          Instructions::BIT(this, 2, A);
          break;
        }
        case 0x58: {
          Instructions::BIT(this, 3, B);
          break;
        }
        case 0x59: {
          Instructions::BIT(this, 3, C);
          break;
        }
        case 0x5A: {
          Instructions::BIT(this, 3, D);
          break;
        }
        case 0x5B: {
          Instructions::BIT(this, 3, E);
          break;
        }
        case 0x5C: {
          Instructions::BIT(this, 3, H);
          break;
        }
        case 0x5D: {
          Instructions::BIT(this, 3, L);
          break;
        }
        case 0x5E: {
          Instructions::BIT(this, 3, bus->read8(HL));
          break;
        }
        case 0x5F: {
          Instructions::BIT(this, 3, A);
          break;
        }

        case 0x60: {
          Instructions::BIT(this, 4, B);
          break;
        }
        case 0x61: {
          Instructions::BIT(this, 4, C);
          break;
        }
        case 0x62: {
          Instructions::BIT(this, 4, D);
          break;
        }
        case 0x63: {
          Instructions::BIT(this, 4, E);
          break;
        }
        case 0x64: {
          Instructions::BIT(this, 4, H);
          break;
        }
        case 0x65: {
          Instructions::BIT(this, 4, L);
          break;
        }
        case 0x66: {
          Instructions::BIT(this, 4, bus->read8(HL));
          break;
        }
        case 0x67: {
          Instructions::BIT(this, 4, A);
          break;
        }

        case 0x68: {
          Instructions::BIT(this, 5, B);
          break;
        }
        case 0x69: {
          Instructions::BIT(this, 5, C);
          break;
        }
        case 0x6A: {
          Instructions::BIT(this, 5, D);
          break;
        }
        case 0x6B: {
          Instructions::BIT(this, 5, E);
          break;
        }
        case 0x6C: {
          Instructions::BIT(this, 5, H);
          break;
        }
        case 0x6D: {
          Instructions::BIT(this, 5, L);
          break;
        }
        case 0x6E: {
          Instructions::BIT(this, 5, bus->read8(HL));
          break;
        }
        case 0x6F: {
          Instructions::BIT(this, 5, A);
          break;
        }

        case 0x70: {
          Instructions::BIT(this, 6, B);
          break;
        }
        case 0x71: {
          Instructions::BIT(this, 6, C);
          break;
        }
        case 0x72: {
          Instructions::BIT(this, 6, D);
          break;
        }
        case 0x73: {
          Instructions::BIT(this, 6, E);
          break;
        }
        case 0x74: {
          Instructions::BIT(this, 6, H);
          break;
        }
        case 0x75: {
          Instructions::BIT(this, 6, L);
          break;
        }
        case 0x76: {
          Instructions::BIT(this, 6, bus->read8(HL));
          break;
        }
        case 0x77: {
          Instructions::BIT(this, 6, A);
          break;
        }

        case 0x78: {
          Instructions::BIT(this, 7, B);
          break;
        }
        case 0x79: {
          Instructions::BIT(this, 7, C);
          break;
        }
        case 0x7A: {
          Instructions::BIT(this, 7, D);
          break;
        }
        case 0x7B: {
          Instructions::BIT(this, 7, E);
          break;
        }
        case 0x7C: {
          Instructions::BIT(this, 7, H);
          break;
        }
        case 0x7D: {
          Instructions::BIT(this, 7, L);
          break;
        }
        case 0x7E: {
          Instructions::BIT(this, 7, bus->read8(HL));
          break;
        }
        case 0x7F: {
          Instructions::BIT(this, 7, A);
          break;
        }

        case 0x80: {
          Instructions::RES(this, 0, B);
          break;
        }
        case 0x81: {
          Instructions::RES(this, 0, C);
          break;
        }
        case 0x82: {
          Instructions::RES(this, 0, D);
          break;
        }
        case 0x83: {
          Instructions::RES(this, 0, E);
          break;
        }
        case 0x84: {
          Instructions::RES(this, 0, H);
          break;
        }
        case 0x85: {
          Instructions::RES(this, 0, L);
          break;
        }
        case 0x86: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 0, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0x87: {
          Instructions::RES(this, 0, A);
          break;
        }

        case 0x88: {
          Instructions::RES(this, 1, B);
          break;
        }
        case 0x89: {
          Instructions::RES(this, 1, C);
          break;
        }
        case 0x8A: {
          Instructions::RES(this, 1, D);
          break;
        }
        case 0x8B: {
          Instructions::RES(this, 1, E);
          break;
        }
        case 0x8C: {
          Instructions::RES(this, 1, H);
          break;
        }
        case 0x8D: {
          Instructions::RES(this, 1, L);
          break;
        }
        case 0x8E: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 1, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0x8F: {
          Instructions::RES(this, 1, A);
          break;
        }

        case 0x90: {
          Instructions::RES(this, 2, B);
          break;
        }
        case 0x91: {
          Instructions::RES(this, 2, C);
          break;
        }
        case 0x92: {
          Instructions::RES(this, 2, D);
          break;
        }
        case 0x93: {
          Instructions::RES(this, 2, E);
          break;
        }
        case 0x94: {
          Instructions::RES(this, 2, H);
          break;
        }
        case 0x95: {
          Instructions::RES(this, 2, L);
          break;
        }
        case 0x96: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 2, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0x97: {
          Instructions::RES(this, 2, A);
          break;
        }

        case 0x98: {
          Instructions::RES(this, 3, B);
          break;
        }
        case 0x99: {
          Instructions::RES(this, 3, C);
          break;
        }
        case 0x9A: {
          Instructions::RES(this, 3, D);
          break;
        }
        case 0x9B: {
          Instructions::RES(this, 3, E);
          break;
        }
        case 0x9C: {
          Instructions::RES(this, 3, H);
          break;
        }
        case 0x9D: {
          Instructions::RES(this, 3, L);
          break;
        }
        case 0x9E: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 3, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0x9F: {
          Instructions::RES(this, 3, A);
          break;
        }

        case 0xA0: {
          Instructions::RES(this, 4, B);
          break;
        }
        case 0xA1: {
          Instructions::RES(this, 4, C);
          break;
        }
        case 0xA2: {
          Instructions::RES(this, 4, D);
          break;
        }
        case 0xA3: {
          Instructions::RES(this, 4, E);
          break;
        }
        case 0xA4: {
          Instructions::RES(this, 4, H);
          break;
        }
        case 0xA5: {
          Instructions::RES(this, 4, L);
          break;
        }
        case 0xA6: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 4, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xA7: {
          Instructions::RES(this, 4, A);
          break;
        }

        case 0xA8: {
          Instructions::RES(this, 5, B);
          break;
        }
        case 0xA9: {
          Instructions::RES(this, 5, C);
          break;
        }
        case 0xAA: {
          Instructions::RES(this, 5, D);
          break;
        }
        case 0xAB: {
          Instructions::RES(this, 5, E);
          break;
        }
        case 0xAC: {
          Instructions::RES(this, 5, H);
          break;
        }
        case 0xAD: {
          Instructions::RES(this, 5, L);
          break;
        }
        case 0xAE: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 5, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xAF: {
          Instructions::RES(this, 5, A);
          break;
        }

        case 0xB0: {
          Instructions::RES(this, 6, B);
          break;
        }
        case 0xB1: {
          Instructions::RES(this, 6, C);
          break;
        }
        case 0xB2: {
          Instructions::RES(this, 6, D);
          break;
        }
        case 0xB3: {
          Instructions::RES(this, 6, E);
          break;
        }
        case 0xB4: {
          Instructions::RES(this, 6, H);
          break;
        }
        case 0xB5: {
          Instructions::RES(this, 6, L);
          break;
        }
        case 0xB6: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 6, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xB7: {
          Instructions::RES(this, 6, A);
          break;
        }

        case 0xB8: {
          Instructions::RES(this, 7, B);
          break;
        }
        case 0xB9: {
          Instructions::RES(this, 7, C);
          break;
        }
        case 0xBA: {
          Instructions::RES(this, 7, D);
          break;
        }
        case 0xBB: {
          Instructions::RES(this, 7, E);
          break;
        }
        case 0xBC: {
          Instructions::RES(this, 7, H);
          break;
        }
        case 0xBD: {
          Instructions::RES(this, 7, L);
          break;
        }
        case 0xBE: {
          u8 _hl = bus->read8(HL);
          Instructions::RES(this, 7, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xBF: {
          Instructions::RES(this, 7, A);
          break;
        }

        case 0xC0: {
          Instructions::SET(this, 0, B);
          break;
        }
        case 0xC1: {
          Instructions::SET(this, 0, C);
          break;
        }
        case 0xC2: {
          Instructions::SET(this, 0, D);
          break;
        }
        case 0xC3: {
          Instructions::SET(this, 0, E);
          break;
        }
        case 0xC4: {
          Instructions::SET(this, 0, H);
          break;
        }
        case 0xC5: {
          Instructions::SET(this, 0, L);
          break;
        }
        case 0xC6: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 0, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xC7: {
          Instructions::SET(this, 0, A);
          break;
        }

        case 0xC8: {
          Instructions::SET(this, 1, B);
          break;
        }
        case 0xC9: {
          Instructions::SET(this, 1, C);
          break;
        }
        case 0xCA: {
          Instructions::SET(this, 1, D);
          break;
        }
        case 0xCB: {
          Instructions::SET(this, 1, E);
          break;
        }
        case 0xCC: {
          Instructions::SET(this, 1, H);
          break;
        }
        case 0xCD: {
          Instructions::SET(this, 1, L);
          break;
        }
        case 0xCE: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 1, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xCF: {
          Instructions::SET(this, 1, A);
          break;
        }

        case 0xD0: {
          Instructions::SET(this, 2, B);
          break;
        }
        case 0xD1: {
          Instructions::SET(this, 2, C);
          break;
        }
        case 0xD2: {
          Instructions::SET(this, 2, D);
          break;
        }
        case 0xD3: {
          Instructions::SET(this, 2, E);
          break;
        }
        case 0xD4: {
          Instructions::SET(this, 2, H);
          break;
        }
        case 0xD5: {
          Instructions::SET(this, 2, L);
          break;
        }
        case 0xD6: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 2, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xD7: {
          Instructions::SET(this, 2, A);
          break;
        }

        case 0xD8: {
          Instructions::SET(this, 3, B);
          break;
        }
        case 0xD9: {
          Instructions::SET(this, 3, C);
          break;
        }
        case 0xDA: {
          Instructions::SET(this, 3, D);
          break;
        }
        case 0xDB: {
          Instructions::SET(this, 3, E);
          break;
        }
        case 0xDC: {
          Instructions::SET(this, 3, H);
          break;
        }
        case 0xDD: {
          Instructions::SET(this, 3, L);
          break;
        }
        case 0xDE: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 3, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xDF: {
          Instructions::SET(this, 3, A);
          break;
        }

        case 0xE0: {
          Instructions::SET(this, 4, B);
          break;
        }
        case 0xE1: {
          Instructions::SET(this, 4, C);
          break;
        }
        case 0xE2: {
          Instructions::SET(this, 4, D);
          break;
        }
        case 0xE3: {
          Instructions::SET(this, 4, E);
          break;
        }
        case 0xE4: {
          Instructions::SET(this, 4, H);
          break;
        }
        case 0xE5: {
          Instructions::SET(this, 4, L);
          break;
        }
        case 0xE6: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 4, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xE7: {
          Instructions::SET(this, 4, A);
          break;
        }

        case 0xE8: {
          Instructions::SET(this, 5, B);
          break;
        }
        case 0xE9: {
          Instructions::SET(this, 5, C);
          break;
        }
        case 0xEA: {
          Instructions::SET(this, 5, D);
          break;
        }
        case 0xEB: {
          Instructions::SET(this, 5, E);
          break;
        }
        case 0xEC: {
          Instructions::SET(this, 5, H);
          break;
        }
        case 0xED: {
          Instructions::SET(this, 5, L);
          break;
        }
        case 0xEE: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 5, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xEF: {
          Instructions::SET(this, 5, A);
          break;
        }

        case 0xF0: {
          Instructions::SET(this, 6, B);
          break;
        }
        case 0xF1: {
          Instructions::SET(this, 6, C);
          break;
        }
        case 0xF2: {
          Instructions::SET(this, 6, D);
          break;
        }
        case 0xF3: {
          Instructions::SET(this, 6, E);
          break;
        }
        case 0xF4: {
          Instructions::SET(this, 6, H);
          break;
        }
        case 0xF5: {
          Instructions::SET(this, 6, L);
          break;
        }
        case 0xF6: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 6, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xF7: {
          Instructions::SET(this, 6, A);
          break;
        }

        case 0xF8: {
          Instructions::SET(this, 7, B);
          break;
        }
        case 0xF9: {
          Instructions::SET(this, 7, C);
          break;
        }
        case 0xFA: {
          Instructions::SET(this, 7, D);
          break;
        }
        case 0xFB: {
          Instructions::SET(this, 7, E);
          break;
        }
        case 0xFC: {
          Instructions::SET(this, 7, H);
          break;
        }
        case 0xFD: {
          Instructions::SET(this, 7, L);
          break;
        }
        case 0xFE: {
          u8 _hl = bus->read8(HL);
          Instructions::SET(this, 7, _hl);
          bus->write8(HL, _hl);
          break;
        }
        case 0xFF: {
          Instructions::SET(this, 7, A);
          break;
        }

        default: {
          // cpu_logger->info("unimplemented CB op: {:#04x}", bus->peek(PC - 1));
          assert(0);
          // exit(-1);
        }
      }
      break;
    }
    case 0xCC: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (get_flag(FLAG::ZERO)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        bus->m_cycle();
      }
      break;
    }
    case 0xCD: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);

      push_to_stack(((PC & 0xFF00) >> 8));
      push_to_stack((PC & 0xFF));

      PC = (high << 8) + low;
      bus->m_cycle();

      break;
    }
    case 0xCF: {
      Instructions::RST(this, 0x8);
      break;
    }
    case 0xCE: {
      Instructions::ADC(this, A, bus->read8(PC++));
      break;
    }
    case 0xD0: {
      bus->m_cycle();
      if (!get_flag(FLAG::CARRY)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD1: {
      Instructions::POP(this, DE);
      break;
    }
    case 0xD2: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD4: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (!get_flag(FLAG::CARRY)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        bus->m_cycle();
      }
      break;
    }
    case 0xD5: {
      Instructions::PUSH(this, DE);
      break;
    }
    case 0xD6: {
      u8 val = bus->read8(PC++);
      if (((A & 0xf) - ((val) & 0xf)) & 0x10) {
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
      Instructions::RST(this, 0x10);
      break;
    }
    case 0xD8: {
      bus->m_cycle();
      if (get_flag(FLAG::CARRY)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD9: {  // RETI
      u8 low  = pull_from_stack();
      u8 high = pull_from_stack();
      bus->m_cycle();
      IME = true;
      PC  = (high << 8) + low;
      break;
    }
    case 0xDA: {
      bus->m_cycle();
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        bus->m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xDC: {
      u8 low  = bus->read8(PC++);
      u8 high = bus->read8(PC++);
      if (get_flag(FLAG::CARRY)) {
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));

        PC = (high << 8) + low;
        bus->m_cycle();
      }
      break;
    }
    case 0xDF: {
      Instructions::RST(this, 0x18);
      break;
    }
    case 0xDE: {
      Instructions::SBC(this, A, bus->read8(PC++));
      break;
    }
    case 0xE0: {
      Instructions::LD_M_R(this, 0xFF00 + bus->read8(PC++), A);
      break;
    }

    case 0xE1: {
      Instructions::POP(this, HL);
      break;
    }
    case 0xE2: {
      Instructions::LD_M_R(this, 0xFF00 + C, A);
      break;
    }
    case 0xE5: {
      Instructions::PUSH(this, HL);
      break;
    }
    case 0xE6: {
      Instructions::AND(this, A, bus->read8(PC++));
      break;
    }
    case 0xE7: {
      Instructions::RST(this, 0x20);
      break;
    }
    case 0xE8: {
      Instructions::ADD_SP_E8(this);
      break;
    }
    case 0xE9: {
      PC = HL;
      break;
    }
    case 0xEA: {
      Instructions::LD_M_R(this, bus->read16(PC++), A);
      PC++;
      break;
    }
    case 0xEE: {
      Instructions::XOR(this, A, bus->read8(PC++));
      break;
    }
    case 0xEF: {
      Instructions::RST(this, 0x28);
      break;
    }
    case 0xF0: {
      Instructions::LD_R_R(A, bus->read8(0xFF00 + bus->read8(PC++)));
      break;
    }
    case 0xF1: {
      F = pull_from_stack() & 0b11110000;
      A = pull_from_stack();
      break;
    }
    case 0xF2: {
      Instructions::LD_R_R(A, bus->read8(0xFF00 + C));
      break;
    }
    case 0xF3: {
      IME = false;
      break;
    }
    case 0xF5: {
      Instructions::PUSH(this, AF);
      break;
    }
    case 0xF6: {
      Instructions::OR(this, A, bus->read8(PC++));
      break;
    }
    case 0xF7: {
      Instructions::RST(this, 0x30);
      break;
    }
    case 0xF8: {
      Instructions::LD_HL_SP_E8(this);
      break;
    }
    case 0xF9: {
      SP = HL;
      bus->m_cycle();
      break;
    }
    case 0xFA: {
      u16 address = bus->read16(PC++);
      Instructions::LD_R_R(A, bus->read8(address));
      PC++;
      break;
    }

    case 0xFB: {
      // set_ime();
      IME = true;
      // ("[CPU] EI");
      break;
    }
    case 0xFE: {
      Instructions::CP(this, A, bus->read8(PC++));
      break;
    }
    case 0xFF: {
      Instructions::RST(this, 0x38);
      break;
    }
    default: {
      assert(0);
    }
  }
}