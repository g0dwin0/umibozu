#pragma once
#include "common.h"
#include "bus.h"
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

  class SharpSM83 {
   private:
    std::vector<u8> wram;

   public:
    SharpSM83();
    ~SharpSM83();
    Bus* bus;

    u8 A = 0x01, B = 0x00, C = 0x13, D = 0x0, E = 0xD8, H = 0x0, L = 0x0;
    u16 AF = 0x0;
    u16 BC = 0x0;
    u16 DE = 0x0;
    u16 HL = 0x0;
    u16 SP = 0xFFFE;
    u16 PC = 0x100;
    
    u8 read8(const u16 address);
    void write8(const u16 address, const u8 value);
    
    void run_instruction();
  };
}  // namespace Umibozu