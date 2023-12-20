#pragma once

#include <array>

#include "bus.h"
#include "common.h"
#include "mapper.h"
#include "ppu.h"
#include "timer.h"
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

namespace Umibozu {

  enum class FLAG { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };
  enum class CPU_STATUS { ACTIVE, HALT_MODE, STOP, PAUSED };

  struct SharpSM83 {
    SharpSM83();
    ~SharpSM83();
    Bus* bus = nullptr;
    struct REG_16 {
      u8& high;
      u8& low;

      REG_16(u8& A, u8& B) : high(A), low(B) {
        high = A;
        low  = B;
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

      size_t operator+(int c) const { return get_value() + c; }
      size_t operator+(REG_16& reg) const {
        return get_value() + reg.get_value();
      }
      REG_16& operator+=(REG_16& reg) {
        u16 val = get_value() + reg.get_value();
        high    = (val & 0xFF00) >> 8;
        low     = val & 0xFF;
        return *this;
      }
      operator u16() { return get_value(); }

      u16 operator&(int val) const { return get_value() & val; };

      REG_16& operator=(u16 val) {
        high = (val & 0xFF00) >> 8;
        low  = val & 0xFF;
        return *this;
      };

      [[nodiscard]] u16 get_value() const { return ((high << 8) + low); }
    };
    u8 A, F, B, C, D, E, H, L;
    REG_16 AF{A, F};
    REG_16 BC{B, C};
    REG_16 DE{D, E};
    REG_16 HL{H, L};
    u16 SP            = 0xFFFE;
    u16 PC            = 0x0100;
    CPU_STATUS status = CPU_STATUS::ACTIVE;
    Timer timer;
    PPU* ppu                                = nullptr;
    Mapper* mapper                          = nullptr;
    bool IME                                = false;
    const std::array<u8, 4> OFFSET_TABLE    = {9, 3, 5, 7};
    const std::array<u8, 5> INTERRUPT_TABLE = {
        VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT,
        JOYPAD_INTERRUPT};
    [[nodiscard]] u8 read8(u16 address);
    [[nodiscard]] u16 read16(u16 address);

    [[nodiscard]] u8 peek(u16 address) const;
    void write8(u16 address, u8 value);

    void push_to_stack(u8 value);
    u8 pull_from_stack();

    void set_flag(FLAG);
    void unset_flag(FLAG);
    [[nodiscard]] u8 get_flag(FLAG) const;
    void handle_system_io_write(u16 address, u8 value);
    u8 handle_system_io_read(u16 address);

    void handle_interrupts();
    void run_instruction();
    void m_cycle();
    [[nodiscard]] std::string get_cpu_mode() const;

   private:
    // TODO: replace u8 refs with REG8 register type
    inline void HALT();
    inline void LD_HL_SP_E8();
    inline void LD_R_R(u8& r_1, u8 r_2);
    inline void LD_R16_U16(REG_16& r_1, u16 val);
    inline void ADD_SP_E8();
    inline void LD_M_R(u16 address, u8 val);
    inline void LD_SP_U16(u16& r_1, u16 val);
    inline void LD_U16_SP(u16 address, u16 sp_val);
    inline void LD_R_AMV(u8& r_1, REG_16& r_16);
    inline void DEC(u8& r);
    inline void SCF();
    inline void NOP();
    inline void DEC_R16(REG_16& r);
    inline void DEC_SP(u16& sp);
    inline void CCF();
    inline void INC(u8& r);
    inline void INC_16(REG_16& r);
    inline void ADD(u8& r, u8 r_2);
    inline void CP(const u8& r, const u8& r_2);
    inline void OR(u8& r, u8 r_2);
    inline void POP(REG_16& r);
    inline void PUSH(REG_16& r);
    inline void RRCA();
    inline void RLCA();
    inline void RLA();
    inline void RST(u8 pc_new);
    inline void ADC(u8& r, u8 r_2);
    inline void SBC(u8& r, u8 r_2);
    inline void SUB(u8& r, u8 r_2);
    inline void AND(u8& r, u8 r_2);
    inline void XOR(u8& r, u8 r_2);
    inline void RLC(u8& r);
    inline void RLC_HL();
    inline void RRC(u8& r);
    inline void RRC_HL();
    inline void SLA(u8& r);
    inline void SRA(u8& r);
    inline void SRA_HL();
    inline void SLA_HL();
    inline void RR(u8& r);
    inline void RL(u8& r);
    inline void RL_HL();
    inline void RR_HL();
    inline void SWAP(u8& r);
    inline void SWAP_HL();
    inline void SRL(u8& r);
    inline void SRL_HL();
    inline void SET(u8 p, u8& r);
    inline void RES(u8 p, u8& r);
    inline void BIT(u8 p, const u8& r);
    inline void STOP();
  };
}  // namespace Umibozu