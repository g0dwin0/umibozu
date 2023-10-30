#pragma once
#include <bitset>

#include "bus.h"
#include "common.h"
#define VBLANK_INTERRUPT 0x40
#define STAT_INTERRUPT 0x48
#define TIMER_INTERRUPT 0x50
#define SERIAL_INTERRUPT 0x58
#define JOYPAD_INTERRUPT 0x60
namespace Umibozu {

  enum class FLAG : u8 { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };

  class SharpSM83 {
   private:
    // std::vector<u8> wram;

   public:
    SharpSM83();
    ~SharpSM83();
    Bus* bus;

    u8 A = 0x1, B = 0x00, C = 0x13, D = 0x0, E = 0xD8, H = 0x1, L = 0x4D;
    u16 AF     = 0xB0;
    u16 BC     = 0x0;
    u16 DE     = 0x0;
    u16 HL     = 0x0;
    u16 SP     = 0xFFFE;
    u16 PC     = 0x100;
    u64 cycles = 0;
    // Interrupt master enable
    u8 IME = 0x0;
    bool tima_to_tma = false;
    static constexpr std::array<u8, 5> interrupt_table = {
        VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT, JOYPAD_INTERRUPT};

    static constexpr std::array<u32, 4> clock_select_table = {4096, 262144, 65536, 16384};

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

    #pragma region INTERRUPTS
    void handle_interrupts();
    void request_interrupt(InterruptType);
    #pragma endregion INTERRUPTS

    void run_instruction();
    void m_cycle();
  };
}  // namespace Umibozu