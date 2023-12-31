#include "cpu.h"

#include <bitset>
#include <cassert>
#include <stdexcept>

#include "bus.h"
#include "common.h"
#include "fmt/core.h"
#include "ppu.h"

using namespace Umibozu;

SM83::SM83()  = default;
SM83::~SM83() = default;

inline void SM83::STOP() {
  return;
}
std::string SM83::get_cpu_mode() const {
  switch (status) {
    case SM83::Status::ACTIVE: {
      return "ACTIVE";
    }
    case SM83::Status::HALT_MODE: {
      return "HALT";
    }
    case SM83::Status::STOP: {
      return "STOP";
    }
    case SM83::Status::PAUSED: {
      return "PAUSED";
    }
  }

  throw std::runtime_error("invalid cpu mode");
}
inline void SM83::HALT() {
  status = Status::HALT_MODE;
  while (status == Status::HALT_MODE) {
    if (bus->io.data[IE] & bus->io.data[IF]) {
      status = Status::ACTIVE;
    }
    m_cycle();
  }
}
inline void SM83::LD_HL_SP_E8() {
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
inline void SM83::LD_R_R(u8& r_1, u8 r_2) { r_1 = r_2; }
inline void SM83::ADD_SP_E8() {
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
  m_cycle();
  SP += val;

  reset_zero();
  reset_negative();
}

// write value to memory address
inline void SM83::LD_M_R(const u16 address, u8 val) { write8(address, val); }

inline void SM83::LD_SP_U16(u16& r_1, u16 val) { r_1 = val; }
inline void SM83::LD_R16_U16(REG_16& r_1, u16 val) { r_1 = val; }
inline void SM83::LD_U16_SP(u16 address, u16 sp_val) {
  write8(address, sp_val & 0xFF);
  write8(address + 1, (sp_val & 0xFF00) >> 8);
}
inline void SM83::LD_R_AMV(u8& r_1, REG_16& r_16) { r_1 = read8(r_16); }
inline void SM83::DEC(u8& r) {
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
inline void SM83::SCF() {
  reset_negative();
  reset_half_carry();
  set_carry();
}
inline void SM83::NOP() {}
inline void SM83::DEC_R16(REG_16& r) {
  r--;
  m_cycle();
}
inline void SM83::DEC_SP(u16& sp) {
  sp--;
  m_cycle();
}
inline void SM83::CCF() {
  reset_negative();
  reset_half_carry();
  if (get_flag(Flag::CARRY)) {
    reset_carry();
  } else {
    set_carry();
  }
}
inline void SM83::INC(u8& r) {
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
inline void SM83::INC_16(REG_16& r) {
  m_cycle();
  r++;
}
inline void SM83::ADD(u8& r, u8 r_2) {
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
inline void SM83::CP(const u8& r, const u8& r_2) {
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

inline void SM83::OR(u8& r, u8 r_2) {
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
inline void SM83::POP(REG_16& r) {
  r.low  = pull_from_stack();
  r.high = pull_from_stack();
}
inline void SM83::PUSH(REG_16& r) {
  m_cycle();
  push_to_stack(r.high);
  push_to_stack(r.low);
}
inline void SM83::RRCA() {
  if (A & 0x1) {
    set_carry();
  } else {
    reset_carry();
  }
  A >>= 1;
  A |= get_flag(Flag::CARRY) ? 0x80 : 0;

  reset_zero();
  reset_negative();
  reset_half_carry();
}
inline void SM83::RLCA() {
  if (A & 0x80) {
    set_carry();
  } else {
    reset_carry();
  }
  A <<= 1;
  A |= get_flag(Flag::CARRY);

  reset_zero();
  reset_negative();
  reset_half_carry();
}
inline void SM83::RLA() {
  if (get_flag(Flag::CARRY)) {
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
inline void SM83::RST(u8 pc_new) {
  m_cycle();
  push_to_stack((PC & 0xFF00) >> 8);
  push_to_stack((PC & 0xFF));
  PC = pc_new;
}
inline void SM83::ADC(u8& r, u8 r_2) {
  u8 carry = get_flag(Flag::CARRY);

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
inline void SM83::SBC(u8& r, u8 r_2) {
  u8 carry = get_flag(Flag::CARRY);

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
inline void SM83::SUB(u8& r, u8 r_2) {
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

inline void SM83::AND(u8& r, u8 r_2) {
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

inline void SM83::XOR(u8& r, u8 r_2) {
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

inline void SM83::RLC(u8& r) {
  if (r & 0x80) {
    set_carry();
  } else {
    reset_carry();
  }
  r <<= 1;
  r |= get_flag(Flag::CARRY);
  if (r == 0) {
    set_zero();
  } else {
    reset_zero();
  }

  reset_negative();
  reset_half_carry();
}

inline void SM83::RLC_HL() {
  u8 _hl = read8(HL);
  RLC(_hl);
  write8(HL, _hl);
}

inline void SM83::RRC(u8& r) {
  if (r & 0x1) {
    set_carry();
  } else {
    reset_carry();
  }
  r >>= 1;
  r |= get_flag(Flag::CARRY) ? 0x80 : 0;
  if (r == 0) {
    set_zero();
  } else {
    reset_zero();
  }

  reset_negative();
  reset_half_carry();
}
inline void SM83::RRC_HL() {
  u8 _hl = read8(HL);
  RRC(_hl);
  write8(HL, _hl);
}
inline void SM83::SLA(u8& r) {
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
inline void SM83::SRA(u8& r) {
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
inline void SM83::SRA_HL() {
  u8 _hl = read8(HL);
  SRA(_hl);
  write8(HL, _hl);
}
inline void SM83::SLA_HL() {
  u8 _hl = read8(HL);
  SLA(_hl);
  write8(HL, _hl);
}

inline void SM83::RR(u8& r) {
  if (get_flag(Flag::CARRY)) {
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

inline void SM83::RL(u8& r) {
  if (get_flag(Flag::CARRY)) {
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

inline void SM83::RL_HL() {
  u8 r = read8(HL);
  RL(r);
  write8(HL, r);
}

inline void SM83::RR_HL() {
  u8 r = read8(HL);
  RR(r);
  write8(HL, r);
}

inline void SM83::SWAP(u8& r) {
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
inline void SM83::SWAP_HL() {
  u8 r = read8(HL);
  SWAP(r);
  write8(HL, r);
}

inline void SM83::SRL(u8& r) {
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
inline void SM83::SRL_HL() {
  u8 r = read8(HL);
  SRL(r);
  write8(HL, r);
}

inline void SM83::SET(u8 p, u8& r) { r |= (1 << p); }
inline void SM83::RES(u8 p, u8& r) { r &= ~(1 << p); }
inline void SM83::BIT(const u8 p, const u8& r) {
  if (r & (1 << p)) {
    reset_zero();
  } else {
    set_zero();
  }

  reset_negative();
  set_half_carry();
}

void SM83::m_cycle() {
#ifndef CPU_TEST_MODE_H
  if (speed == Speed::DOUBLE) {
    timer.div += 2;
    ppu->tick(2);
  } else {
    timer.div += 4;
    ppu->tick(4);
  }
#endif

  if (timer.overflow_update_queued) {
    timer.overflow_update_queued = false;
    timer.counter                = timer.modulo;
    bus->request_interrupt(InterruptType::TIMER);
  }
  if (timer.ticking_enabled) {

    u16 div_bits = (timer.div & (1 << TIMER_BIT[bus->io.data[TAC] & 0x3])) >>
                   TIMER_BIT[bus->io.data[TAC] & 0x3];
    // fmt::println("Div bits: {:d}", div_bits);
    u8 te_bit = (bus->io.data[TAC] & (1 << 2)) >> 2;

    u8 n_val = div_bits & te_bit;

    if (n_val == 0 && timer.prev_and_result == 1) {
      if (timer.counter == 0xFF) {
        timer.overflow_update_queued = true;
      }
      timer.counter++;
    }
    timer.prev_and_result = n_val;
  }
}
u8 SM83::handle_system_io_read(const u16 address) {
  if (address != (0xFF00 + SB) && address != (0xFF00 + SC) &&
      address != (0xFF00 + LY) && address != (0xFF00 + LCDC) &&
      address != (0xFF00 + JOYPAD)) {
    // fmt::println("[R] register address: {:#04x}, value: {:#04x}", address,
    //              bus->io.data[address - 0xFF00]);
  }

  switch (address - 0xFF00) {
    case JOYPAD: {
      u8 method_bits = (bus->io.data[JOYPAD] & 0x30) >> 4;

      switch (method_bits) {
        case 0x1: {
          return bus->joypad.get_buttons();
        }
        case 0x2: {
          return bus->joypad.get_dpad();
        }
        case 0x3: {
          return 0xF;
        }
        default: {
          assert(false);
        }
      }

      break;
    }
    case DIV: {
      return timer.get_div();
    }
    case TIMA: {
      return timer.counter;
    }
    case TMA: {
      return timer.modulo;
    }
    case LY: {
      if (ppu->lcdc.lcd_ppu_enable == 0) {
        return 0;
      }
      break;
    }
    case NR52: {
      return 0;
    }
    case STAT: {
      if (ppu->lcdc.lcd_ppu_enable == 0) {
        return 0;
      }
      break;
    }
    case SVBK: {
      fmt::println("SVBK: {:#04x}", bus->io.data[SVBK]);
      break;
    }
    case VBK: {
      fmt::println("VBK: {:#04x}", bus->io.data[VBK]);
      break;
    }
    case KEY1: {
      fmt::println("returning key1: {:#04x}", (u8)speed);
      return (u8)speed;
    }
    case HDMA5: {
      fmt::println("HDMA5 read while active: {:08b}", bus->io.data[HDMA5]);
      break;
    }
  }

  return bus->io.read8(address - 0xFF00);
}
u8 SM83::read8(const u16 address) {
#ifdef CPU_TEST_MODE_H
  return test_memory[address];
#endif
  m_cycle();
  // fmt::println("[R] {:#04x}", address);
  if (address >= 0xFE00 && address <= 0xFE9F && (u8)ppu->get_mode() >= 2) {
    return 0xFF;
  }

  if ((address >= 0xFF00 && address <= 0xFF7F) || address == 0xFFFF) {
    // fmt::println("io write");
    return handle_system_io_read(address);
  };
  return mapper->read8(address);
};

u16 SM83::read16(const u16 address) {
  u8 low  = read8(address);
  u8 high = read8(address + 1);
  PC++;
  return (high << 8) + low;
}
// debug only!
u8 SM83::peek(const u16 address) const { return mapper->read8(address); }
void SM83::init_hdma(u16 length) {
  ppu->hdma_index     = 0;
  bus->io.data[HDMA5] = (length / 0x10) - 1;
};
void SM83::terminate_hdma() {
  fmt::println("HDMA terminated early.");
  bus->io.data[HDMA5] &= ~(1 << 7);

  fmt::println("HDMA5: {:08b}", bus->io.data[HDMA5]);
};

void SM83::handle_system_io_write(const u16 address, const u8 value) {
  if (address != (0xFF00 + SB) && address != (0xFF00 + SC) &&
      address != (0xFF00 + LY) && address != (0xFF00 + LCDC) &&
      address != (0xFF00 + JOYPAD)) {
    // fmt::println("[W] register address: {:#04x}, value: {:#04x}", address,
    //              value);
  }
  switch (address - 0xFF00) {
    case JOYPAD: {
      if (value == 0x30) {  // BUTTONS NOR D-PAD SELECTED
        bus->io.data[JOYPAD] = 0xFF;
        return;
      }
      break;
    }
    case SC: {
      // DEPRECATED: doesn't have much use except for logging; remove when color
      // is implemented & working
      if (value == 0x81) {
        // bus->serial_port_buffer[bus->serial_port_index++] =
        // bus->wram.data[SB]; std::string str_data(bus->serial_port_buffer,
        // SERIAL_PORT_BUFFER_SIZE); fmt::println("serial data: {}", str_data);
      }
      if (value == 0x01) {
        fmt::println("transfer completed");
        bus->request_interrupt(InterruptType::SERIAL);
      }
      break;
    }
    case DIV: {
      timer.div = 0x0;
      break;
    }
    case TIMA: {
      timer.counter = value;
      break;
    }
    case TMA: {
      timer.modulo = value;
      break;
    }
    case TAC: {
      timer.set_tac(value);
      // fmt::println("TAC: {:08b}", value);
      break;
    }
    case LCDC: {
      if ((bus->io.data[LCDC] & 0x80) == 0 && (value & 0x80)) {
        ppu->frame_skip = true;
        ppu->frame.clear();
        bus->io.data[LY] = 0;
      }
      ppu->lcdc = value;
      break;
    }
    case STAT: {
      fmt::println("STAT: {:08b}", value);
      u8 read_only_bits  = value & 0x78;
      bus->io.data[STAT] = (1<<7) + read_only_bits + (u8)ppu->get_mode();
      return;
    }
    case LY: {  // read only
      return;
    }
    case LYC: {
      bus->io.data[LYC] = value;
      bus->io.data[STAT] &= 0xfb;
      bus->io.data[STAT] |=
          (bus->io.data[LY] == bus->io.data[LYC]) ? (1 << 2) : 0;

      if ((bus->io.data[STAT] & 1 << 6) && (bus->io.data[STAT] & 1 << 2)) {
        bus->request_interrupt(InterruptType::LCD);
      }

      break;
    }
    case DMA: {
      u16 address = (value << 8);

      for (size_t i = 0; i < 0xA0; i++) {
        bus->oam.data[i] = read8(address + i);
      }
      break;
    }
    case BGP: {
      // fmt::println("OBP0 write: {:08b}", value);
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->sys_palettes.BGP[0][0] = ppu->shade_table[id_0];
      ppu->sys_palettes.BGP[0][1] = ppu->shade_table[id_1];
      ppu->sys_palettes.BGP[0][2] = ppu->shade_table[id_2];
      ppu->sys_palettes.BGP[0][3] = ppu->shade_table[id_3];

      //      fmt::println("BGP0[0] = {:#16x}", ppu->sys_palettes.BGP[0][0]);
      //      fmt::println("BGP0[1] = {:#16x}", ppu->sys_palettes.BGP[0][1]);
      //      fmt::println("BGP0[2] = {:#16x}", ppu->sys_palettes.BGP[0][2]);
      //      fmt::println("BGP0[3] = {:#16x}", ppu->sys_palettes.BGP[0][3]);
      break;
    }
    case OBP0: {
      // fmt::println("OBP0 write: {:08b}", value);
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->sys_palettes.OBP[0][0] = ppu->shade_table[id_0];
      ppu->sys_palettes.OBP[0][1] = ppu->shade_table[id_1];
      ppu->sys_palettes.OBP[0][2] = ppu->shade_table[id_2];
      ppu->sys_palettes.OBP[0][3] = ppu->shade_table[id_3];
      break;
    }
    case OBP1: {
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->sys_palettes.OBP[1][0] = ppu->shade_table[id_0];
      ppu->sys_palettes.OBP[1][1] = ppu->shade_table[id_1];
      ppu->sys_palettes.OBP[1][2] = ppu->shade_table[id_2];
      ppu->sys_palettes.OBP[1][3] = ppu->shade_table[id_3];
      break;
    }
    case VBK: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }

      bus->vbk  = (value & 0x1);
      bus->vram = &bus->vram_banks[bus->vbk];

      bus->io.data[VBK] = 0xFE + bus->vbk;
      return;
    }
    case SVBK: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }

      bus->svbk = (value & 0x7);
      if (bus->svbk == 0) {
        bus->svbk = 1;
      }

      bus->wram = &bus->wram_banks[bus->svbk];
      
      bus->io.data[SVBK] = 0xF8 + bus->svbk;
      return;
    }
    case KEY1: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }
      fmt::println("value: {:#04x}", value);
      fmt::println("desired speed: {:#04x}", value & 0x80);
      fmt::println("current speed: {:#04x}", bus->io.data[KEY1] & 0x80);

      u8 current_speed = (bus->io.data[KEY1] & 0x80);
      u8 desired_speed = (value & 0x80);

      if (current_speed != desired_speed) {
        bus->io.data[KEY1] = desired_speed;
        speed              = (Speed)desired_speed;
        timer.div          = 0;
        fmt::println("speed switched");
        return;
      }

      return;
    }
    case HDMA1:
    case HDMA2:
    case HDMA3:
    case HDMA4: {
      break;
    }

    case HDMA5: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }
      fmt::println("entered HDMA5, value: {:#08b}", value);

      if ((bus->io.data[HDMA5] & 0x80) == 0 && (value & 0x80) == 0x80) {
        fmt::println("terminated HDMA");
        return terminate_hdma();
      }

      u16 length = (1 + (value & 0x7f)) * 0x10;
      u16 src    = ((bus->io.data[HDMA1] << 8) + bus->io.data[HDMA2]) & 0xfff0;
      u16 dst    = ((bus->io.data[HDMA3] << 8) + bus->io.data[HDMA4]) & 0x1ff0;

      fmt::println("[HDMA] src:     {:#16x}", src);
      fmt::println("[HDMA] dst:     {:#16x} ({:#04x})", dst,
                   VRAM_ADDRESS_OFFSET + dst);
      fmt::println("[HDMA] length:  {:#16x}", length);

      if ((value & 0x80) == 0) {  // GDMA
        for (size_t index = 0; index < length; index++) {
          u16 src_address = (src + index);
          u8 src_data     = mapper->read8(src_address);

          u16 vram_address = ((dst + index));

          if (vram_address > 0x1FFF) {
            fmt::println("skipped GDMA");
            bus->io.data[HDMA5] = 0xFF;
            exit(-1);
            return;
          }
          fmt::println(
              "[GDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: {:#16x} ",
              src_address, src_data, 0x8000 + vram_address);

          bus->vram->write8(vram_address, src_data);
        }
        bus->io.data[HDMA5] = 0xFF;
        return;
      } else {  // hblank dma
        if ((bus->io.data[HDMA5] & 0x80) == 0x80) {
          init_hdma(length);
        }
        return;
      }
      break;
    }

    case BCPS: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }

      bus->bcps.address        = value & 0b111111;
      bus->bcps.auto_increment = (value & (1 << 7)) >> 7;

      break;
    }
    case BCPD: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }
      bus->bg_palette_ram.data[bus->bcps.address] = value;
      if (bus->bcps.auto_increment) {
        bus->bcps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low =
              bus->bg_palette_ram.data.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high =
              (bus->bg_palette_ram.data.at((palette_id * 8) + (color * 2) + 1));

          u16 f_color = (p_high << 8) + p_low;

          ppu->sys_palettes.BGP[palette_id][color] = f_color;
        }
      }
      break;
    }
    case OCPS: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }
      bus->ocps.address        = value & 0x3f;
      bus->ocps.auto_increment = (value & (1 << 7)) >> 7;

      break;
    }
    case OCPD: {
      if (bus->mode != COMPAT_MODE::CGB_ONLY) {
        return;
      }

      bus->obj_palette_ram.data[bus->ocps.address] = value;
      if (bus->ocps.auto_increment) {
        bus->ocps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low =
              bus->obj_palette_ram.data.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high = (bus->obj_palette_ram.data.at((palette_id * 8) +
                                                    (color * 2) + 1));


          ppu->sys_palettes.OBP[palette_id][color] = (p_high << 8) + p_low;
        }
      }
      break;
    }
  }
  return bus->io.write8(address - 0xFF00, value);
};
void SM83::write8(const u16 address, const u8 value) {
#ifdef CPU_TEST_MODE_H
  test_memory[address] = value;
  return;
#endif
  m_cycle();
  // fmt::println("write requested {:#04x}", address);
  if (address >= 0xFE00 && address <= 0xFE9F && (u8)ppu->get_mode() >= 2) {
    return;
  };
  if ((address >= 0xFF00 && address <= 0xFF7F) || address == 0xFFFF) {
    // fmt::println("io write");
    return handle_system_io_write(address, value);
  };
  mapper->write8(address, value);
}
void SM83::push_to_stack(const u8 value) { write8(--SP, value); }

u8 SM83::pull_from_stack() { return read8(SP++); }

void SM83::set_flag(Flag flag) { F |= (1 << (u8)flag); };

void SM83::unset_flag(Flag flag) { F &= ~(1 << (u8)flag); };

u8 SM83::get_flag(Flag flag) const { return F & (1 << (u8)flag) ? 1 : 0; }

void SM83::handle_interrupts() {
  // fmt::println("[HANDLE INTERRUPTS] IF:   {:08b}", bus->wram.data[IF]);
  // fmt::println("[HANDLE INTERRUPTS] IE:   {:08b}", bus->wram.data[IE]);
  if (IME && bus->io.data[IE] && bus->io.data[IF]) {
    // fmt::println("[HANDLE INTERRUPTS] IME:  {:08b}", IME);

    std::bitset<8> ie_set(bus->io.data[IE]);
    std::bitset<8> if_set(bus->io.data[IF]);

    for (size_t i = 0; i < 5; i++) {
      if (if_set[i] && ie_set[i]) {
        IME = false;

        // fmt::println("handling interrupt: {:#04x}", i);
        // fmt::println("[INTERRUPT HANDLER] PC: {:#04x}", PC);
        if_set[i].flip();
        // bus->io.data[IF] &= ~(1 << i);
        m_cycle();
        m_cycle();
        push_to_stack(((PC & 0xFF00) >> 8));
        push_to_stack((PC & 0xFF));
        PC = INTERRUPT_TABLE[i];
        m_cycle();
        // fmt::println("[INTERRUPT HANDLER] AFTER PC: {:#04x}", PC);
      };
    }

    bus->io.data[IE] = ie_set.to_ulong();
    bus->io.data[IF] = if_set.to_ulong();
    // Bit 0 (VBlank) has the highest priority, and Bit 4 (Joypad) has the
    // lowest priority.
  }
};
void SM83::run_instruction() {
#ifndef CPU_TEST_MODE_H
  // if (mapper == nullptr) {
  //   throw std::runtime_error("mapper error");
  // }

  if (status == Status::PAUSED) {
    return;
  }
  handle_interrupts();
  // fmt::println(
  //     "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H:
  //     {:02X}L: {:02X} SP: {:04X} PC: {:02X}:{:04X} ({:02X} {:02X} {:02X}
  //     {:02X}) TE: {}", A, F, B, C, D, E, H, L, SP, mapper->rom_bank, PC,
  //     peek(PC), peek(PC +
  // 1), peek(PC + 2), peek(PC + 3), timer.ticking_enabled);
#endif

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
    case 0x2: {
      LD_M_R(BC, A);
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
    case 0xA: {
      A = read8(BC);
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
    case 0x10: {
      STOP();
      PC++;
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
      i8 offset = read8(PC++);
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
    case 0x1F: {  // make func
      if (get_flag(Flag::CARRY)) {
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
      if (!get_flag(Flag::ZERO)) {
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
      if (get_flag(Flag::HALF_CARRY) ||
          (!get_flag(Flag::NEGATIVE) && (A & 0xf) > 9)) {
        adjustment |= 0x6;
      }

      if (get_flag(Flag::CARRY) || (!get_flag(Flag::NEGATIVE) && A > 0x99)) {
        adjustment |= 0x60;
        set_carry();
      }

      if (get_flag(Flag::NEGATIVE)) {
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
      if (get_flag(Flag::ZERO)) {
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
      if (!get_flag(Flag::CARRY)) {
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
    case 0x34: {
      u8 hl = read8(HL);
      INC(hl);
      write8(HL, hl);
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
      if (get_flag(Flag::CARRY)) {
        m_cycle();
        PC = PC + offset;
      }
      break;
    }
    case 0x39: {
      m_cycle();

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
      A = read8(HL--);
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
    case 0x52: {
      // LD_R_R(D, D);
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
    case 0x76: {
      HALT();
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
      if (!get_flag(Flag::ZERO)) {
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
      if (!get_flag(Flag::ZERO)) {
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
      if (!get_flag(Flag::ZERO)) {
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
      if (get_flag(Flag::ZERO)) {
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
      if (get_flag(Flag::ZERO)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xCB: {
      switch (read8(PC++)) {
        case 0x0: {
          RLC(B);
          break;
        }

        case 0x1: {
          RLC(C);
          break;
        }

        case 0x2: {
          RLC(D);
          break;
        }

        case 0x3: {
          RLC(E);
          break;
        }

        case 0x4: {
          RLC(H);
          break;
        }

        case 0x5: {
          RLC(L);
          break;
        }

        case 0x6: {
          RLC_HL();
          break;
        }

        case 0x7: {
          RLC(A);
          break;
        }

        case 0x8: {
          RRC(B);
          break;
        }
        case 0x9: {
          RRC(C);
          break;
        }
        case 0xA: {
          RRC(D);
          break;
        }
        case 0xB: {
          RRC(E);
          break;
        }
        case 0xC: {
          RRC(H);
          break;
        }
        case 0xD: {
          RRC(L);
          break;
        }
        case 0xE: {
          RRC_HL();
          break;
        }
        case 0xF: {
          RRC(A);
          break;
        }

        case 0x10: {
          RL(B);
          break;
        }
        case 0x11: {
          RL(C);
          break;
        }
        case 0x12: {
          RL(D);
          break;
        }
        case 0x13: {
          RL(E);
          break;
        }
        case 0x14: {
          RL(H);
          break;
        }
        case 0x15: {
          RL(L);
          break;
        }
        case 0x16: {
          RL_HL();
          break;
        }
        case 0x17: {
          RL(A);
          break;
        }

        case 0x18: {
          RR(B);
          break;
        }
        case 0x19: {
          RR(C);
          break;
        }
        case 0x1A: {
          RR(D);
          break;
        }
        case 0x1B: {
          RR(E);
          break;
        }
        case 0x1C: {
          RR(H);
          break;
        }
        case 0x1D: {
          RR(L);
          break;
        }
        case 0x1E: {
          RR_HL();
          break;
        }
        case 0x1F: {
          RR(A);
          break;
        }

        case 0x20: {
          SLA(B);
          break;
        }
        case 0x21: {
          SLA(C);
          break;
        }
        case 0x22: {
          SLA(D);
          break;
        }
        case 0x23: {
          SLA(E);
          break;
        }
        case 0x24: {
          SLA(H);
          break;
        }
        case 0x25: {
          SLA(L);
          break;
        }
        case 0x26: {
          SLA_HL();
          break;
        }
        case 0x27: {
          SLA(A);
          break;
        }
        case 0x28: {
          SRA(B);
          break;
        }
        case 0x29: {
          SRA(C);
          break;
        }
        case 0x2A: {
          SRA(D);
          break;
        }
        case 0x2B: {
          SRA(E);
          break;
        }
        case 0x2C: {
          SRA(H);
          break;
        }
        case 0x2D: {
          SRA(L);
          break;
        }
        case 0x2E: {
          SRA_HL();
          break;
        }
        case 0x2F: {
          SRA(A);
          break;
        }
        case 0x30: {
          SWAP(B);
          break;
        }
        case 0x31: {
          SWAP(C);
          break;
        }
        case 0x32: {
          SWAP(D);
          break;
        }
        case 0x33: {
          SWAP(E);
          break;
        }
        case 0x34: {
          SWAP(H);
          break;
        }
        case 0x35: {
          SWAP(L);
          break;
        }
        case 0x36: {
          SWAP_HL();
          break;
        }
        case 0x37: {
          SWAP(A);
          break;
        }

        case 0x38: {
          SRL(B);
          break;
        }

        case 0x39: {
          SRL(C);
          break;
        }

        case 0x3A: {
          SRL(D);
          break;
        }

        case 0x3B: {
          SRL(E);
          break;
        }

        case 0x3C: {
          SRL(H);
          break;
        }

        case 0x3D: {
          SRL(L);
          break;
        }

        case 0x3E: {
          SRL_HL();
          break;
        }

        case 0x3F: {
          SRL(A);
          break;
        }

        case 0x40: {
          BIT(0, B);
          break;
        }
        case 0x41: {
          BIT(0, C);
          break;
        }
        case 0x42: {
          BIT(0, D);
          break;
        }
        case 0x43: {
          BIT(0, E);
          break;
        }
        case 0x44: {
          BIT(0, H);
          break;
        }
        case 0x45: {
          BIT(0, L);
          break;
        }
        case 0x46: {
          BIT(0, read8(HL));
          break;
        }
        case 0x47: {
          BIT(0, A);
          break;
        }

        case 0x48: {
          BIT(1, B);
          break;
        }
        case 0x49: {
          BIT(1, C);
          break;
        }
        case 0x4A: {
          BIT(1, D);
          break;
        }
        case 0x4B: {
          BIT(1, E);
          break;
        }
        case 0x4C: {
          BIT(1, H);
          break;
        }
        case 0x4D: {
          BIT(1, L);
          break;
        }
        case 0x4E: {
          BIT(1, read8(HL));
          break;
        }
        case 0x4F: {
          BIT(1, A);
          break;
        }

        case 0x50: {
          BIT(2, B);
          break;
        }
        case 0x51: {
          BIT(2, C);
          break;
        }
        case 0x52: {
          BIT(2, D);
          break;
        }
        case 0x53: {
          BIT(2, E);
          break;
        }
        case 0x54: {
          BIT(2, H);
          break;
        }
        case 0x55: {
          BIT(2, L);
          break;
        }
        case 0x56: {
          BIT(2, read8(HL));
          break;
        }
        case 0x57: {
          BIT(2, A);
          break;
        }
        case 0x58: {
          BIT(3, B);
          break;
        }
        case 0x59: {
          BIT(3, C);
          break;
        }
        case 0x5A: {
          BIT(3, D);
          break;
        }
        case 0x5B: {
          BIT(3, E);
          break;
        }
        case 0x5C: {
          BIT(3, H);
          break;
        }
        case 0x5D: {
          BIT(3, L);
          break;
        }
        case 0x5E: {
          BIT(3, read8(HL));
          break;
        }
        case 0x5F: {
          BIT(3, A);
          break;
        }

        case 0x60: {
          BIT(4, B);
          break;
        }
        case 0x61: {
          BIT(4, C);
          break;
        }
        case 0x62: {
          BIT(4, D);
          break;
        }
        case 0x63: {
          BIT(4, E);
          break;
        }
        case 0x64: {
          BIT(4, H);
          break;
        }
        case 0x65: {
          BIT(4, L);
          break;
        }
        case 0x66: {
          BIT(4, read8(HL));
          break;
        }
        case 0x67: {
          BIT(4, A);
          break;
        }

        case 0x68: {
          BIT(5, B);
          break;
        }
        case 0x69: {
          BIT(5, C);
          break;
        }
        case 0x6A: {
          BIT(5, D);
          break;
        }
        case 0x6B: {
          BIT(5, E);
          break;
        }
        case 0x6C: {
          BIT(5, H);
          break;
        }
        case 0x6D: {
          BIT(5, L);
          break;
        }
        case 0x6E: {
          BIT(5, read8(HL));
          break;
        }
        case 0x6F: {
          BIT(5, A);
          break;
        }

        case 0x70: {
          BIT(6, B);
          break;
        }
        case 0x71: {
          BIT(6, C);
          break;
        }
        case 0x72: {
          BIT(6, D);
          break;
        }
        case 0x73: {
          BIT(6, E);
          break;
        }
        case 0x74: {
          BIT(6, H);
          break;
        }
        case 0x75: {
          BIT(6, L);
          break;
        }
        case 0x76: {
          BIT(6, read8(HL));
          break;
        }
        case 0x77: {
          BIT(6, A);
          break;
        }

        case 0x78: {
          BIT(7, B);
          break;
        }
        case 0x79: {
          BIT(7, C);
          break;
        }
        case 0x7A: {
          BIT(7, D);
          break;
        }
        case 0x7B: {
          BIT(7, E);
          break;
        }
        case 0x7C: {
          BIT(7, H);
          break;
        }
        case 0x7D: {
          BIT(7, L);
          break;
        }
        case 0x7E: {
          BIT(7, read8(HL));
          break;
        }
        case 0x7F: {
          BIT(7, A);
          break;
        }

        case 0x80: {
          RES(0, B);
          break;
        }
        case 0x81: {
          RES(0, C);
          break;
        }
        case 0x82: {
          RES(0, D);
          break;
        }
        case 0x83: {
          RES(0, E);
          break;
        }
        case 0x84: {
          RES(0, H);
          break;
        }
        case 0x85: {
          RES(0, L);
          break;
        }
        case 0x86: {
          u8 _hl = read8(HL);
          RES(0, _hl);
          write8(HL, _hl);
          break;
        }
        case 0x87: {
          RES(0, A);
          break;
        }

        case 0x88: {
          RES(1, B);
          break;
        }
        case 0x89: {
          RES(1, C);
          break;
        }
        case 0x8A: {
          RES(1, D);
          break;
        }
        case 0x8B: {
          RES(1, E);
          break;
        }
        case 0x8C: {
          RES(1, H);
          break;
        }
        case 0x8D: {
          RES(1, L);
          break;
        }
        case 0x8E: {
          u8 _hl = read8(HL);
          RES(1, _hl);
          write8(HL, _hl);
          break;
        }
        case 0x8F: {
          RES(1, A);
          break;
        }

        case 0x90: {
          RES(2, B);
          break;
        }
        case 0x91: {
          RES(2, C);
          break;
        }
        case 0x92: {
          RES(2, D);
          break;
        }
        case 0x93: {
          RES(2, E);
          break;
        }
        case 0x94: {
          RES(2, H);
          break;
        }
        case 0x95: {
          RES(2, L);
          break;
        }
        case 0x96: {
          u8 _hl = read8(HL);
          RES(2, _hl);
          write8(HL, _hl);
          break;
        }
        case 0x97: {
          RES(2, A);
          break;
        }

        case 0x98: {
          RES(3, B);
          break;
        }
        case 0x99: {
          RES(3, C);
          break;
        }
        case 0x9A: {
          RES(3, D);
          break;
        }
        case 0x9B: {
          RES(3, E);
          break;
        }
        case 0x9C: {
          RES(3, H);
          break;
        }
        case 0x9D: {
          RES(3, L);
          break;
        }
        case 0x9E: {
          u8 _hl = read8(HL);
          RES(3, _hl);
          write8(HL, _hl);
          break;
        }
        case 0x9F: {
          RES(3, A);
          break;
        }

        case 0xA0: {
          RES(4, B);
          break;
        }
        case 0xA1: {
          RES(4, C);
          break;
        }
        case 0xA2: {
          RES(4, D);
          break;
        }
        case 0xA3: {
          RES(4, E);
          break;
        }
        case 0xA4: {
          RES(4, H);
          break;
        }
        case 0xA5: {
          RES(4, L);
          break;
        }
        case 0xA6: {
          u8 _hl = read8(HL);
          RES(4, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xA7: {
          RES(4, A);
          break;
        }

        case 0xA8: {
          RES(5, B);
          break;
        }
        case 0xA9: {
          RES(5, C);
          break;
        }
        case 0xAA: {
          RES(5, D);
          break;
        }
        case 0xAB: {
          RES(5, E);
          break;
        }
        case 0xAC: {
          RES(5, H);
          break;
        }
        case 0xAD: {
          RES(5, L);
          break;
        }
        case 0xAE: {
          u8 _hl = read8(HL);
          RES(5, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xAF: {
          RES(5, A);
          break;
        }

        case 0xB0: {
          RES(6, B);
          break;
        }
        case 0xB1: {
          RES(6, C);
          break;
        }
        case 0xB2: {
          RES(6, D);
          break;
        }
        case 0xB3: {
          RES(6, E);
          break;
        }
        case 0xB4: {
          RES(6, H);
          break;
        }
        case 0xB5: {
          RES(6, L);
          break;
        }
        case 0xB6: {
          u8 _hl = read8(HL);
          RES(6, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xB7: {
          RES(6, A);
          break;
        }

        case 0xB8: {
          RES(7, B);
          break;
        }
        case 0xB9: {
          RES(7, C);
          break;
        }
        case 0xBA: {
          RES(7, D);
          break;
        }
        case 0xBB: {
          RES(7, E);
          break;
        }
        case 0xBC: {
          RES(7, H);
          break;
        }
        case 0xBD: {
          RES(7, L);
          break;
        }
        case 0xBE: {
          u8 _hl = read8(HL);
          RES(7, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xBF: {
          RES(7, A);
          break;
        }

        case 0xC0: {
          SET(0, B);
          break;
        }
        case 0xC1: {
          SET(0, C);
          break;
        }
        case 0xC2: {
          SET(0, D);
          break;
        }
        case 0xC3: {
          SET(0, E);
          break;
        }
        case 0xC4: {
          SET(0, H);
          break;
        }
        case 0xC5: {
          SET(0, L);
          break;
        }
        case 0xC6: {
          u8 _hl = read8(HL);
          SET(0, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xC7: {
          SET(0, A);
          break;
        }

        case 0xC8: {
          SET(1, B);
          break;
        }
        case 0xC9: {
          SET(1, C);
          break;
        }
        case 0xCA: {
          SET(1, D);
          break;
        }
        case 0xCB: {
          SET(1, E);
          break;
        }
        case 0xCC: {
          SET(1, H);
          break;
        }
        case 0xCD: {
          SET(1, L);
          break;
        }
        case 0xCE: {
          u8 _hl = read8(HL);
          SET(1, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xCF: {
          SET(1, A);
          break;
        }

        case 0xD0: {
          SET(2, B);
          break;
        }
        case 0xD1: {
          SET(2, C);
          break;
        }
        case 0xD2: {
          SET(2, D);
          break;
        }
        case 0xD3: {
          SET(2, E);
          break;
        }
        case 0xD4: {
          SET(2, H);
          break;
        }
        case 0xD5: {
          SET(2, L);
          break;
        }
        case 0xD6: {
          u8 _hl = read8(HL);
          SET(2, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xD7: {
          SET(2, A);
          break;
        }

        case 0xD8: {
          SET(3, B);
          break;
        }
        case 0xD9: {
          SET(3, C);
          break;
        }
        case 0xDA: {
          SET(3, D);
          break;
        }
        case 0xDB: {
          SET(3, E);
          break;
        }
        case 0xDC: {
          SET(3, H);
          break;
        }
        case 0xDD: {
          SET(3, L);
          break;
        }
        case 0xDE: {
          u8 _hl = read8(HL);
          SET(3, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xDF: {
          SET(3, A);
          break;
        }

        case 0xE0: {
          SET(4, B);
          break;
        }
        case 0xE1: {
          SET(4, C);
          break;
        }
        case 0xE2: {
          SET(4, D);
          break;
        }
        case 0xE3: {
          SET(4, E);
          break;
        }
        case 0xE4: {
          SET(4, H);
          break;
        }
        case 0xE5: {
          SET(4, L);
          break;
        }
        case 0xE6: {
          u8 _hl = read8(HL);
          SET(4, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xE7: {
          SET(4, A);
          break;
        }

        case 0xE8: {
          SET(5, B);
          break;
        }
        case 0xE9: {
          SET(5, C);
          break;
        }
        case 0xEA: {
          SET(5, D);
          break;
        }
        case 0xEB: {
          SET(5, E);
          break;
        }
        case 0xEC: {
          SET(5, H);
          break;
        }
        case 0xED: {
          SET(5, L);
          break;
        }
        case 0xEE: {
          u8 _hl = read8(HL);
          SET(5, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xEF: {
          SET(5, A);
          break;
        }

        case 0xF0: {
          SET(6, B);
          break;
        }
        case 0xF1: {
          SET(6, C);
          break;
        }
        case 0xF2: {
          SET(6, D);
          break;
        }
        case 0xF3: {
          SET(6, E);
          break;
        }
        case 0xF4: {
          SET(6, H);
          break;
        }
        case 0xF5: {
          SET(6, L);
          break;
        }
        case 0xF6: {
          u8 _hl = read8(HL);
          SET(6, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xF7: {
          SET(6, A);
          break;
        }

        case 0xF8: {
          SET(7, B);
          break;
        }
        case 0xF9: {
          SET(7, C);
          break;
        }
        case 0xFA: {
          SET(7, D);
          break;
        }
        case 0xFB: {
          SET(7, E);
          break;
        }
        case 0xFC: {
          SET(7, H);
          break;
        }
        case 0xFD: {
          SET(7, L);
          break;
        }
        case 0xFE: {
          u8 _hl = read8(HL);
          SET(7, _hl);
          write8(HL, _hl);
          break;
        }
        case 0xFF: {
          SET(7, A);
          break;
        }

        default: {
          fmt::println("[CPU] unimplemented CB op: {:#04x}", peek(PC - 1));
          // exit(-1);
        }
      }
      break;
    }
    case 0xCC: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(Flag::ZERO)) {
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
      if (!get_flag(Flag::CARRY)) {
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
      if (!get_flag(Flag::CARRY)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD4: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (!get_flag(Flag::CARRY)) {
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
      RST(0x10);
      break;
    }
    case 0xD8: {
      m_cycle();
      if (get_flag(Flag::CARRY)) {
        u8 low  = pull_from_stack();
        u8 high = pull_from_stack();
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xD9: {  // RETI
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
      if (get_flag(Flag::CARRY)) {
        m_cycle();
        PC = (high << 8) + low;
      }
      break;
    }
    case 0xDC: {
      u8 low  = read8(PC++);
      u8 high = read8(PC++);
      if (get_flag(Flag::CARRY)) {
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
      F = pull_from_stack() & 0b11110000;
      A = pull_from_stack();
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
      u16 address = read16(PC++);
      LD_R_R(A, read8(address));
      break;
    }

    case 0xFB: {
      // set_ime();
      IME = true;
      // fmt::println("EI");
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
      throw std::runtime_error(
          fmt::format("[CPU] unimplemented opcode: {:#04x}", opcode));
    }
  }
}