#pragma once
#include <bitset>
#include <cstddef>
#include <stdexcept>

#include "bus.h"
#include "common.h"

#define set_zero() \
  { set_flag(FLAG::ZERO); };
#define set_negative() \
  { set_flag(FLAG::NEGATIVE); };
#define set_half_carry() \
  { set_flag(FLAG::HALF_CARRY); };
#define set_carry() \
  { set_flag(FLAG::CARRY); };
#define reset_zero() \
  { unset_flag(FLAG::ZERO); };
#define reset_negative() \
  { unset_flag(FLAG::NEGATIVE); };
#define reset_half_carry() \
  { unset_flag(FLAG::HALF_CARRY); };
#define reset_carry() \
  { unset_flag(FLAG::CARRY); };
#define VBLANK_INTERRUPT 0x40
#define STAT_INTERRUPT 0x48
#define TIMER_INTERRUPT 0x50
#define SERIAL_INTERRUPT 0x58
#define JOYPAD_INTERRUPT 0x60
#define OP_NOT_IMPL(r)                                 \
  fmt::println("[CPU] not implemented: {}", __func__); \
  exit(-1);
namespace Umibozu {

  enum class FLAG : u8 { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };
  enum class FLAG_STATE_MODIFIER { NOT, IS };
  class SharpSM83 {
   private:
    // std::vector<u8> wram;

   public:
    SharpSM83();
    ~SharpSM83();
    Bus* bus;
    struct REG_16 {
      u8& high;
      u8& low;

      REG_16(u8& _A, u8& _B) : high(_A), low(_B) {
        high = _A;
        low  = _B;
      };

      REG_16& operator++() {
        if (low == 0xFF) {
          high++;
        }
        low++;
        return *this;
      };

      u16 operator++(int) {
        u16 v = get_value();
        ++*this;
        return v;
      };

      REG_16& operator--() {
        if (low == 0x00) {
          high--;
        }
        low--;
        return *this;
      };
      u16 operator--(int) {
        u16 v = get_value();
        --*this;
        return v;
      };

      size_t operator+(int c) { return get_value() + c; }
      size_t operator+(REG_16& reg) { return get_value() + reg.get_value(); }
      REG_16& operator+=(REG_16& reg) {
        u16 val = get_value() + reg.get_value();
        high    = (val & 0xFF00) >> 8;
        low     = val & 0xFF;
        return *this;
      }
      operator u16() { return get_value(); }

      u16 operator&(int val) { return get_value() & val; };

      REG_16& operator=(u16 val) {
        high = (val & 0xFF00) >> 8;
        low  = val & 0xFF;
        return *this;
      };

      u16 get_value() { return ((high << 8) + low); }
    };
    u8 A, F, B, C, D, E, H, L;
    REG_16 AF  = REG_16(A, F);
    REG_16 BC  = REG_16(B, C);
    REG_16 DE  = REG_16(D, E);
    REG_16 HL  = REG_16(H, L);
    u16 SP     = 0xFFFE;
    u16 PC     = 0x101;
    u64 cycles = 0;
    // Interrupt master enable
    u8 IME                                             = 0x0;
    bool tima_to_tma                                   = false;
    static constexpr std::array<u8, 5> interrupt_table = {
        VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT,
        JOYPAD_INTERRUPT};

    static constexpr std::array<u32, 4> clock_select_table = {4096, 262144,
                                                              65536, 16384};

    u8 read8(const u16 address);
    u16 read16(const u16 address);

    u8 peek(const u16 address);

    void write8(const u16 address, const u8 value);

    void push_to_stack(const u8 value);
    u8 pull_from_stack();

    void set_flag(FLAG);
    u8 get_flag(FLAG);
    void unset_flag(FLAG);

    void handle_interrupts();
    void request_interrupt(InterruptType);

    void run_instruction();
    void m_cycle();
    inline void LD_HL_SP_E8() {
      u8 op  = read8(PC++);
      i8 val = op;

      if (((SP & 0xFF) + op) > 0xFF) {
        set_carry();
      } else {
        reset_carry();
      }

      if (((SP & 0xf) + (op & 0xf)) > 0xf) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      HL = SP + val;

      m_cycle();

      reset_zero();
      reset_negative();
    }
    inline void LD_R_R(u8& r_1, u8 r_2) { r_1 = r_2; };
    inline void ADD_SP_E8() {
      u8 op  = read8(PC++);
      i8 val = op;
      m_cycle();

      if (((SP & 0xFF) + op) > 0xFF) {
        set_carry();
      } else {
        reset_carry();
      }

      if (((SP & 0xf) + (op & 0xf)) > 0xf) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      SP += val;

      reset_zero();
      reset_negative();
    }

    // load value into memory address
    inline void LD_M_R(const u16 address, u8 val) { write8(address, val); }

    inline void LD_SP_U16(u16& r_1, u16 val) { r_1 = val; };
    inline void LD_R16_U16(REG_16& r_1, u16 val) { r_1 = val; };
    inline void LD_U16_SP(u16 address, u16 sp_val) {
      write8(address, sp_val & 0xFF);
      write8(address + 1, (sp_val & 0xFF00) >> 8);
    }
    inline void LD_R_AMV(u8& r_1, REG_16& r_16) { r_1 = read8(r_16); }
    inline void DEC(u8& r) {
      if (((r & 0xf) - (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      r--;
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      set_negative();
    }
    inline void SCF() {
      reset_negative();
      reset_half_carry();
      set_carry();
    }
    inline void NOP(){};
    inline void DEC_R16(REG_16& r) {
      r--;
      m_cycle();
    }
    inline void DEC_SP(u16& sp) {
      sp--;
      m_cycle();
    }
    inline void CCF() {
      reset_negative();
      reset_half_carry();
      if (get_flag(FLAG::CARRY)) {
        reset_carry();
      } else {
        set_carry();
      }
    }
    inline void INC(u8& r) {
      if (((r & 0xf) + (1 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      r++;
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
    }
    inline void INC_16(REG_16& r) {
      m_cycle();
      r++;
    };
    inline void ADD(u8& r, u8 r_2) {
      if (((r & 0xf) + (r_2 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if ((r + r_2) > 0xFF) {
        set_carry();
      } else {
        reset_carry();
      }

      r += r_2;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
    }
    inline void CP(const u8& r, const u8& r_2) {
      if ((r - r_2) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      if (((r & 0xf) - (r_2 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }
      if ((r - r_2) == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      set_negative();
    }

    inline void OR(u8& r, u8 r_2) {
      r = r | r_2;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
    }
    inline void POP(REG_16& r) {
      r.low  = pull_from_stack();
      r.high = pull_from_stack();
    }
    inline void PUSH(REG_16& r) {
      m_cycle();
      push_to_stack(r.high);
      push_to_stack(r.low);
    }
    inline void RRCA() {
      if (A & 0x1) {
        set_carry();
      } else {
        reset_carry();
      }
      A >>= 1;
      A |= get_flag(FLAG::CARRY) ? 0x80 : 0;

      reset_zero();
      reset_negative();
      reset_half_carry();
    }
    inline void RLCA() {
      if (A & 0x80) {
        set_carry();
      } else {
        reset_carry();
      }
      A <<= 1;
      A |= get_flag(FLAG::CARRY);

      reset_zero();
      reset_negative();
      reset_half_carry();
    }
    inline void RLA() {
      if (get_flag(FLAG::CARRY)) {
        if (A & 0x80) {
          A <<= 1;
          A += 0x1;
          set_carry();
        } else {
          A <<= 1;
          A += 0x1;
          reset_carry();
        }
      } else {
        if (A & 0x80) {
          A <<= 1;
          set_carry();
        } else {
          A <<= 1;
          reset_carry();
        }
      }

      reset_zero();
      reset_negative();
      reset_half_carry();
    }
    inline void RST(u8 pc_new) {
      m_cycle();
      push_to_stack((PC & 0xFF00) >> 8);
      push_to_stack((PC & 0xFF));
      PC = pc_new;
    }
    inline void ADC(u8& r, u8 r_2) {
      u8 carry = get_flag(FLAG::CARRY);

      if (((r & 0xf) + (r_2 & 0xf) + carry) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if ((r + r_2 + carry) > 0xFF) {
        set_carry();
      } else {
        reset_carry();
      }

      r = r + r_2 + carry;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
    }
    inline void SBC(u8& r, u8 r_2) {
      u8 carry = get_flag(FLAG::CARRY);

      if (((r & 0xf) - (r_2 & 0xf) - carry) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if ((r - r_2 - carry) < 0) {
        set_carry();
      } else {
        reset_carry();
      }

      r = r - r_2 - carry;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      set_negative();
    }
    inline void SUB(u8& r, u8 r_2) {
      if (((r & 0xf) - (r_2 & 0xf)) & 0x10) {
        set_half_carry();
      } else {
        reset_half_carry();
      }

      if ((r - r_2) < 0) {
        set_carry();
      } else {
        reset_carry();
      }
      r = r - r_2;
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      set_negative();
    }

    inline void AND(u8& r, u8 r_2) {
      r = r & r_2;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      set_half_carry();
      reset_carry();
    }

    inline void XOR(u8& r, u8 r_2) {
      r ^= r_2;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
      reset_carry();
    }

    inline void RLC(u8& r) {
      if (r & 0x80) {
        set_carry();
      } else {
        reset_carry();
      }
      r <<= 1;
      r |= get_flag(FLAG::CARRY);
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }

    inline void RLC_HL() {
      u8 _hl = read8(HL);
      RLC(_hl);
      write8(HL, _hl);
    }

    inline void RRC(u8& r) {
      if (r & 0x1) {
        set_carry();
      } else {
        reset_carry();
      }
      r >>= 1;
      r |= get_flag(FLAG::CARRY) ? 0x80 : 0;
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }
    inline void RRC_HL() {
      u8 _hl = read8(HL);
      RRC(_hl);
      write8(HL, _hl);
    }
    inline void SLA(u8& r) {
      if (r & 0x80) {
        set_carry();
      } else {
        reset_carry();
      }
      r <<= 1;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }
    inline void SRA(u8& r) {
      u8 msb = r & 0x80;
      if (r & 0x1) {
        set_carry();
      } else {
        reset_carry();
      }
      r >>= 1;
      r += msb;
      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }
    inline void SRA_HL() {
      u8 _hl = read8(HL);
      SRA(_hl);
      write8(HL, _hl);
    }
    inline void SLA_HL() {
      u8 _hl = read8(HL);
      SLA(_hl);
      write8(HL, _hl);
    }

    inline void RR(u8& r) {
      if (get_flag(FLAG::CARRY)) {
        if (r & 0x1) {
          r >>= 1;
          r += 0x80;
          set_carry();
        } else {
          r >>= 1;
          r += 0x80;
          reset_carry();
        }
      } else {
        if (r & 0x1) {
          r >>= 1;
          set_carry();
        } else {
          r >>= 1;
          reset_carry();
        }
      }

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }

    inline void RL(u8& r) {
      if (get_flag(FLAG::CARRY)) {
        if (r & 0x80) {
          r <<= 1;
          r += 0x1;
          set_carry();
        } else {
          r <<= 1;
          r += 0x1;
          reset_carry();
        }
      } else {
        if (r & 0x80) {
          r <<= 1;
          set_carry();
        } else {
          r <<= 1;
          reset_carry();
        }
      }

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }

      reset_negative();
      reset_half_carry();
    }

    inline void RL_HL() {
      u8 r = read8(HL);
      RL(r);
      write8(HL, r);
    }

    inline void RR_HL() {
      u8 r = read8(HL);
      RR(r);
      write8(HL, r);
    }

    inline void SWAP(u8& r) {
      u8 hi = (r & 0xF0);
      u8 lo = r & 0xF;

      r = (lo << 4) + (hi >> 4);

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_carry();
      reset_half_carry();
      reset_negative();
    }
    inline void SWAP_HL() {
      u8 r = read8(HL);
      SWAP(r);
      write8(HL, r);
    }

    inline void SRL(u8& r) {
      if (r & 0x1) {
        set_carry();
      } else {
        reset_carry();
      }
      r >>= 1;

      if (r == 0) {
        set_zero();
      } else {
        reset_zero();
      }
      reset_negative();
      reset_half_carry();
    }
    inline void SRL_HL() {
      u8 r = read8(HL);
      SRL(r);
      write8(HL, r);
    }

    inline void SET(u8 p,u8& r) { r |= (1 << p); }
    inline void RES(u8 p, u8& r) { r &= ~(1 << p); }
    inline void BIT(const u8 p, const u8& r) {
      if (r & (1 << p)) {
        reset_zero();
      } else {
        set_zero();
      }

      reset_negative();
      set_half_carry();
    }
  };
}  // namespace Umibozu