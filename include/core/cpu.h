#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "bus.h"
#include "common.h"
#include "mapper.h"
#include "ppu.h"
#include "timer.h"

#define VBLANK_INTERRUPT 0x40
#define STAT_INTERRUPT 0x48
#define TIMER_INTERRUPT 0x50
#define SERIAL_INTERRUPT 0x58
#define JOYPAD_INTERRUPT 0x60

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



    struct REG_16 {
      u8 &high;
      u8 &low;

      REG_16(u8 &A, u8 &B) : high(A), low(B) {};

      REG_16 &operator++() {
        if (low == 0xFF) { high++; }
        low++;
        return *this;
      };

      u16 operator++(int) {
        u16 v = get_value();
        ++*this;
        return v;
      };

      REG_16 &operator--() {
        if (low == 0x00) { high--; }
        low--;
        return *this;
      };
      u16 operator--(int) {
        u16 v = get_value();
        --*this;
        return v;
      };

      size_t operator+(int c) const { return get_value() + c; }
      size_t operator+(REG_16 &reg) const {
        return get_value() + reg.get_value();
      }
      REG_16 &operator+=(REG_16 &reg) {
        u16 val = get_value() + reg.get_value();
        high    = (val & 0xFF00) >> 8;
        low     = val & 0xFF;
        return *this;
      }
      operator u16() { return get_value(); }

      u16 operator&(int val) const { return get_value() & val; };

      REG_16 &operator=(u16 val) {
        high = (val & 0xFF00) >> 8;
        low  = val & 0xFF;
        return *this;
      };

      [[nodiscard]] u16 get_value() const { return ((high << 8) + low); }
    };

    const std::array<u8, 5> INTERRUPT_TABLE = {
        VBLANK_INTERRUPT, STAT_INTERRUPT, TIMER_INTERRUPT, SERIAL_INTERRUPT,
        JOYPAD_INTERRUPT};

    // Registers
    u8 A, F, B, C, D, E, H, L;
    REG_16 AF{A, F};
    REG_16 BC{B, C};
    REG_16 DE{D, E};
    REG_16 HL{H, L};
    u16 SP        = 0xFFFE;
    u16 PC        = 0x0100;
    STATUS status = STATUS::PAUSED;
    bool IME      = false;
    SPEED speed   = SPEED::NORMAL;

    // REFACTOR: make reads go through bus
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
    void handle_system_io_write(const u16 address, const u8 value);
    u8 handle_system_io_read(const u16 address);

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