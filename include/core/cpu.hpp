#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "bus.hpp"
#include "common.hpp"
#include "mapper.hpp"
#include "ppu.hpp"
#include "timer.hpp"

static constexpr u8 VBLANK_INTERRUPT = 0x40;
static constexpr u8 STAT_INTERRUPT   = 0x48;
static constexpr u8 TIMER_INTERRUPT  = 0x50;
static constexpr u8 SERIAL_INTERRUPT = 0x58;
static constexpr u8 JOYPAD_INTERRUPT = 0x60;

static constexpr std::array<u8, 5> IRQ_VECTOR_TABLE = {VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT, JOYPAD_INTERRUPT};

namespace Umibozu {

  struct SM83 {
    enum class FLAG { CARRY = 4, HALF_CARRY = 5, NEGATIVE = 6, ZERO = 7 };
    enum class STATUS { ACTIVE, HALT_MODE, STOP, PAUSED };
    enum class SPEED { NORMAL = 0x00, DOUBLE = 0x80 };

    SM83();
    ~SM83();

    Bus *bus = nullptr;

    std::unordered_map<SM83::STATUS, std::string> cpu_mode = {
        {   SM83::STATUS::ACTIVE, "ACTIVE"},
        {SM83::STATUS::HALT_MODE,   "HALT"},
        {     SM83::STATUS::STOP,   "STOP"},
        {   SM83::STATUS::PAUSED, "PAUSED"},
    };

    bool ei_queued = false;

    // Registers
    union {
      u16 AF;
      struct {
        u8 F;
        u8 A;
      };
    };

    union {
      u16 BC;
      struct {
        u8 C;
        u8 B;
      };
    };

    union {
      u16 DE;
      struct {
        u8 E;
        u8 D;
      };
    };

    union {
      u16 HL;
      struct {
        u8 L;
        u8 H;
      };
    };

    u16 SP        = 0xFFFE;
    u16 PC        = 0x0100;
    STATUS status = STATUS::PAUSED;
    bool IME      = false;
    SPEED speed   = SPEED::NORMAL;

    PPU *ppu       = nullptr;
    Mapper *mapper = nullptr;
    Timer *timer   = nullptr;

    // State
    [[nodiscard]] std::string get_cpu_mode_string() const { return cpu_mode.at(status); };

    // Memory R/W
    [[nodiscard]] u8 read8(const u16 address);
    [[nodiscard]] u16 read16(const u16 address);
    [[nodiscard]] u8 peek(const u16 address) const;
    void write8(const u16 address, const u8 value);
    void push_to_stack(const u8 value);
    u8 pull_from_stack();
    void io_write(const u16 address, const u8 value);
    u8 io_read(const u16 address);

    // CPU Internals
    void run_instruction();
    void handle_interrupts();
    void m_cycle();

    // Flags
    void set_flag(const FLAG);
    void unset_flag(const FLAG);
    [[nodiscard]] u8 get_flag(const FLAG) const;
    void set_zero() { set_flag(Umibozu::SM83::FLAG::ZERO); }
    void set_negative() { set_flag(Umibozu::SM83::FLAG::NEGATIVE); }
    void set_half_carry() { set_flag(Umibozu::SM83::FLAG::HALF_CARRY); }
    void set_carry() { set_flag(Umibozu::SM83::FLAG::CARRY); }
    void reset_zero() { unset_flag(Umibozu::SM83::FLAG::ZERO); }
    void reset_negative() { unset_flag(Umibozu::SM83::FLAG::NEGATIVE); }
    void reset_half_carry() { unset_flag(Umibozu::SM83::FLAG::HALF_CARRY); }
    void reset_carry() { unset_flag(Umibozu::SM83::FLAG::CARRY); }

    // HDMA (GBC)
    void init_hdma(u16 length);
    void terminate_hdma();

#ifdef CPU_TEST_MODE_H
    std::vector<u8> test_memory;
#endif
  };
}  // namespace Umibozu