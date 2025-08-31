#include "instructions.hpp"

#include "bus.hpp"
#include "cpu.hpp"

namespace Instructions {
  using Umibozu::SM83;
  void HALT(SM83 *c) {
    c->status             = SM83::STATUS::HALT_MODE;
    c->bus->cpu_is_halted = true;
    // fmt::println("halt entered");
    while (c->status == SM83::STATUS::HALT_MODE) {
      if (c->bus->io[IE] & c->bus->io[IF]) {
        c->status             = SM83::STATUS::ACTIVE;
        c->bus->cpu_is_halted = false;
      }
      c->m_cycle();
    }
    // fmt::println("halt exit");
  }
  void RRA(SM83 *c) {
    if (c->get_flag(Umibozu::SM83::FLAG::CARRY)) {
      if (c->A & 0x1) {
        c->A >>= 1;
        c->A += 0x80;
        c->set_carry();
      } else {
        c->A >>= 1;
        c->A += 0x80;
        c->reset_carry();
      }
    } else {
      if (c->A & 0x1) {
        c->A >>= 1;
        c->set_carry();
      } else {
        c->A >>= 1;
        c->reset_carry();
      }
    }

    if (c->A == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_zero();
    c->reset_negative();
    c->reset_half_carry();
  }

  void DAA(SM83 *c) {
    // WTF is the DAA instruction?
    // https://ehaskins.com/2018-01-30%20Z80%20DAA/
    u8 adjustment = 0;
    if (c->get_flag(Umibozu::SM83::FLAG::HALF_CARRY) || (!c->get_flag(Umibozu::SM83::FLAG::NEGATIVE) && (c->A & 0xf) > 9)) {
      adjustment |= 0x6;
    }

    if (c->get_flag(Umibozu::SM83::FLAG::CARRY) || (!c->get_flag(Umibozu::SM83::FLAG::NEGATIVE) && c->A > 0x99)) {
      adjustment |= 0x60;
      c->set_carry();
    }

    if (c->get_flag(Umibozu::SM83::FLAG::NEGATIVE)) {
      c->A += -adjustment;
    } else {
      c->A += adjustment;
    }

    c->A &= 0xff;

    if (c->A == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_half_carry();
  }
  void ADD_HL_DE(SM83 *c) {
    if (((c->HL & 0xfff) + (c->DE & 0xfff)) & 0x1000) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    if ((c->HL + c->DE) > 0xFFFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    c->HL = c->HL + c->DE;
    c->H  = (c->HL & 0xFF00) >> 8;
    c->L  = (c->HL & 0xFF);
    c->m_cycle();
    c->reset_negative();
  }
  void ADD_HL_BC(SM83 *c) {
    if (((c->HL & 0xfff) + (c->BC & 0xfff)) & 0x1000) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    if ((c->HL + c->BC) > 0xFFFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    c->HL = c->HL + c->BC;
    c->H  = (c->HL & 0xFF00) >> 8;
    c->L  = (c->HL & 0xFF);
    c->m_cycle();
    c->reset_negative();
  }
  void LD_HL_SP_E8(SM83 *c) {
    u8 op  = c->read8(c->PC++);
    i8 val = op;

    if (((c->SP & 0xFF) + op) > 0xFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }

    if (((c->SP & 0xf) + (op & 0xf)) > 0xf) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }

    c->HL = c->SP + val;

    c->m_cycle();

    c->reset_zero();
    c->reset_negative();
  }
  void LD_R_R(u8 &r_1, u8 r_2) { r_1 = r_2; }

  void STOP(SM83 *c) {
    if (c->bus->mode != SYSTEM_MODE::CGB) return;




    return;
  }
  void ADD_SP_E8(SM83 *c) {
    u8 op  = c->read8(c->PC++);
    i8 val = op;
    c->m_cycle();

    if (((c->SP & 0xFF) + op) > 0xFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }

    if (((c->SP & 0xf) + (op & 0xf)) > 0xf) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    c->m_cycle();
    c->SP += val;

    c->reset_zero();
    c->reset_negative();
  }

  // write value to memory address
  void LD_M_R(SM83 *c, const u16 address, u8 val) { c->write8(address, val); }

  void LD_SP_U16(SM83 *c, u16 val) { c->SP = val; }
  void LD_R16_U16(SM83 *, u16 &r_1, u16 val) { r_1 = val; }
  void LD_U16_SP(SM83 *c, u16 address, u16 sp_val) {
    c->write8(address, sp_val & 0xFF);
    c->write8(address + 1, (sp_val & 0xFF00) >> 8);
  }
  void LD_R_AMV(SM83 *c, u8 &r_1, u16 &r_16) { r_1 = c->read8(r_16); }
  void DEC(SM83 *c, u8 &r) {
    if (((r & 0xf) - (1 & 0xf)) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    r--;
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->set_negative();
  }
  void SCF(SM83 *c) {
    c->reset_negative();
    c->reset_half_carry();
    c->set_carry();
  }
  void NOP() { return; }
  void DEC_R16(SM83 *c, u16 &r) {
    r--;
    c->m_cycle();
  }
  void DEC_SP(SM83 *c) {
    c->SP--;
    c->m_cycle();
  }
  void CCF(SM83 *c) {
    c->reset_negative();
    c->reset_half_carry();
    if (c->get_flag(Umibozu::SM83::FLAG::CARRY)) {
      c->reset_carry();
    } else {
      c->set_carry();
    }
  }
  void INC(SM83 *c, u8 &r) {
    if (((r & 0xf) + (1 & 0xf)) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    r++;
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_negative();
  }
  void INC_16(SM83 *c, u16 &r) {
    c->m_cycle();
    r++;
  }
  void ADD(SM83 *c, u8 &r, u8 r_2) {
    if (((r & 0xf) + (r_2 & 0xf)) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }

    if ((r + r_2) > 0xFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }

    r += r_2;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_negative();
  }
  void CP(SM83 *c, const u8 &r, const u8 &r_2) {
    if ((r - r_2) < 0) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    if (((r & 0xf) - (r_2 & 0xf)) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }
    if ((r - r_2) == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->set_negative();
  }

  void OR(SM83 *c, u8 &r, u8 r_2) {
    r = r | r_2;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
    c->reset_carry();
  }
  void POP(SM83 *c, u16 &r) {
    r = 0;

    r |= c->pull_from_stack();
    r |= (c->pull_from_stack() << 8);

    // r.low = c->pull_from_stack();
    // r.high = c->pull_from_stack();
  }
  void PUSH(SM83 *c, u16 &r) {
    c->m_cycle();
    c->push_to_stack(r >> 8);
    c->push_to_stack(r & 0xFF);
  }
  void RRCA(SM83 *c) {
    if (c->A & 0x1) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    c->A >>= 1;
    c->A |= c->get_flag(Umibozu::SM83::FLAG::CARRY) ? 0x80 : 0;

    c->reset_zero();
    c->reset_negative();
    c->reset_half_carry();
  }
  void RLCA(SM83 *c) {
    if (c->A & 0x80) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    c->A <<= 1;
    c->A |= c->get_flag(Umibozu::SM83::FLAG::CARRY);

    c->reset_zero();
    c->reset_negative();
    c->reset_half_carry();
  }
  void RLA(SM83 *c) {
    if (c->get_flag(Umibozu::SM83::FLAG::CARRY)) {
      if (c->A & 0x80) {
        c->A <<= 1;
        c->A += 0x1;
        c->set_carry();
      } else {
        c->A <<= 1;
        c->A += 0x1;
        c->reset_carry();
      }
    } else {
      if (c->A & 0x80) {
        c->A <<= 1;
        c->set_carry();
      } else {
        c->A <<= 1;
        c->reset_carry();
      }
    }

    c->reset_zero();
    c->reset_negative();
    c->reset_half_carry();
  }
  void RST(SM83 *c, u8 pc_new) {
    c->m_cycle();
    c->push_to_stack((c->PC & 0xFF00) >> 8);
    c->push_to_stack((c->PC & 0xFF));
    c->PC = pc_new;
  }
  void ADC(SM83 *c, u8 &r, u8 r_2) {
    u8 carry = c->get_flag(Umibozu::SM83::FLAG::CARRY);

    if (((r & 0xf) + (r_2 & 0xf) + carry) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }

    if ((r + r_2 + carry) > 0xFF) {
      c->set_carry();
    } else {
      c->reset_carry();
    }

    r = r + r_2 + carry;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_negative();
  }
  void SBC(SM83 *c, u8 &r, u8 r_2) {
    u8 carry = c->get_flag(Umibozu::SM83::FLAG::CARRY);

    if (((r & 0xf) - (r_2 & 0xf) - carry) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }

    if ((r - r_2 - carry) < 0) {
      c->set_carry();
    } else {
      c->reset_carry();
    }

    r = r - r_2 - carry;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->set_negative();
  }
  void SUB(SM83 *c, u8 &r, u8 r_2) {
    if (((r & 0xf) - (r_2 & 0xf)) & 0x10) {
      c->set_half_carry();
    } else {
      c->reset_half_carry();
    }

    if ((r - r_2) < 0) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r = r - r_2;
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->set_negative();
  }

  void AND(SM83 *c, u8 &r, u8 r_2) {
    r = r & r_2;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->set_half_carry();
    c->reset_carry();
  }

  void XOR(SM83 *c, u8 &r, u8 r_2) {
    r ^= r_2;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
    c->reset_carry();
  }

  void RLC(SM83 *c, u8 &r) {
    if (r & 0x80) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r <<= 1;
    r |= c->get_flag(Umibozu::SM83::FLAG::CARRY);
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }

  void RLC_HL(SM83 *c) {
    u8 _hl = c->read8(c->HL);
    Instructions::RLC(c, _hl);
    c->write8(c->HL, _hl);
  }

  void RRC(SM83 *c, u8 &r) {
    if (r & 0x1) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r >>= 1;
    r |= c->get_flag(Umibozu::SM83::FLAG::CARRY) ? 0x80 : 0;
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }
  void RRC_HL(SM83 *c) {
    u8 _hl = c->read8(c->HL);
    Instructions::RRC(c, _hl);
    c->write8(c->HL, _hl);
  }
  void SLA(SM83 *c, u8 &r) {
    if (r & 0x80) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r <<= 1;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }
  void SRA(SM83 *c, u8 &r) {
    u8 msb = r & 0x80;
    if (r & 0x1) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r >>= 1;
    r += msb;
    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }
  void SRA_HL(SM83 *c) {
    u8 _hl = c->read8(c->HL);
    Instructions::SRA(c, _hl);
    c->write8(c->HL, _hl);
  }
  void SLA_HL(SM83 *c) {
    u8 _hl = c->read8(c->HL);
    Instructions::SLA(c, _hl);
    c->write8(c->HL, _hl);
  }

  void RR(SM83 *c, u8 &r) {
    if (c->get_flag(Umibozu::SM83::FLAG::CARRY)) {
      if (r & 0x1) {
        r >>= 1;
        r += 0x80;
        c->set_carry();
      } else {
        r >>= 1;
        r += 0x80;
        c->reset_carry();
      }
    } else {
      if (r & 0x1) {
        r >>= 1;
        c->set_carry();
      } else {
        r >>= 1;
        c->reset_carry();
      }
    }

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }

  void RL(SM83 *c, u8 &r) {
    if (c->get_flag(Umibozu::SM83::FLAG::CARRY)) {
      if (r & 0x80) {
        r <<= 1;
        r += 0x1;
        c->set_carry();
      } else {
        r <<= 1;
        r += 0x1;
        c->reset_carry();
      }
    } else {
      if (r & 0x80) {
        r <<= 1;
        c->set_carry();
      } else {
        r <<= 1;
        c->reset_carry();
      }
    }

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }

    c->reset_negative();
    c->reset_half_carry();
  }

  void RL_HL(SM83 *c) {
    u8 r = c->read8(c->HL);
    Instructions::RL(c, r);
    c->write8(c->HL, r);
  }

  void RR_HL(SM83 *c) {
    u8 r = c->read8(c->HL);
    Instructions::RR(c, r);
    c->write8(c->HL, r);
  }

  void SWAP(SM83 *c, u8 &r) {
    u8 hi = (r & 0xF0);
    u8 lo = r & 0xF;

    r = (lo << 4) + (hi >> 4);

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_carry();
    c->reset_half_carry();
    c->reset_negative();
  }
  void SWAP_HL(SM83 *c) {
    u8 r = c->read8(c->HL);
    Instructions::SWAP(c, r);
    c->write8(c->HL, r);
  }

  void SRL(SM83 *c, u8 &r) {
    if (r & 0x1) {
      c->set_carry();
    } else {
      c->reset_carry();
    }
    r >>= 1;

    if (r == 0) {
      c->set_zero();
    } else {
      c->reset_zero();
    }
    c->reset_negative();
    c->reset_half_carry();
  }
  void SRL_HL(SM83 *c) {
    u8 r = c->read8(c->HL);
    Instructions::SRL(c, r);
    c->write8(c->HL, r);
  }

  void SET(SM83 *, u8 p, u8 &r) { r |= (1 << p); }
  void RES(SM83 *, u8 p, u8 &r) { r &= ~(1 << p); }
  void BIT(SM83 *c, const u8 p, const u8 &r) {
    if (r & (1 << p)) {
      c->reset_zero();
    } else {
      c->set_zero();
    }

    c->reset_negative();
    c->set_half_carry();
  }

}  // namespace Instructions