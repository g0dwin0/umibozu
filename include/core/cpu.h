#pragma once
#include <bitset>

#include "bus.h"
#include "common.h"
namespace Umibozu {

  enum class HARDWARE_REG : u16 {
    JOYPAD = 0xFF00,
    SB     = 0xFF01,
    SC     = 0xFF02,
    DIV    = 0xFF04,
    TIMA   = 0xFF05,
    TMA    = 0xFF06,
    TAC    = 0xFF07,
    IF     = 0xFF0F,
  };

  enum class FLAG : u8 { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };
  class SharpSM83 {
   private:
    std::vector<u8> wram;

   public:
    SharpSM83();
    ~SharpSM83();
    Bus* bus;

    u8 A = 0x01, B = 0x00, C = 0x13, D = 0x0, E = 0xD8, H = 0x1, L = 0x4D;
    u16 AF     = 0xB0;
    u16 BC     = 0x0;
    u16 DE     = 0x0;
    u16 HL     = 0x0;
    u16 SP     = 0xFFFE;
    u16 PC     = 0x100;
    u32 cycles = 0;

    // Interrupt master enable
    bool IME = 0x1;

    u8 read8(const u16 address);
    u8 peek(const u16 address);
    
    void write8(const u16 address, const u8 value);

    void push_to_stack(const u8 value);
    u8 pull_from_stack();

    void set_flag(FLAG);
    u8 get_flag(FLAG);
    
    void unset_flag(FLAG);
    
    void set_zero();
    void set_negative();
    void set_half_carry();
    void set_carry();
    void reset_zero();
    void reset_negative();
    void reset_half_carry();
    void reset_carry();

    void run_instruction();
    void m_cycle();
  };
}  // namespace Umibozu