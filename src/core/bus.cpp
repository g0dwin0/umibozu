#include "core/bus.hpp"

#include "common.hpp"

void Bus::request_interrupt(INTERRUPT_TYPE t) { io[IF] |= (1 << (u8)t); }

std::string Bus::get_mode_string() const {
  if (mode == SYSTEM_MODE::CGB) {
    return "CGB";
  } else {
    return "DMG";
  }
}

std::string Bus::get_label(u16 addr) {
  addr &= 0xFF;

  auto reg = static_cast<IO_REG>(addr);

  if (IO_LABEL_MAP.contains(reg)) return IO_LABEL_MAP.at(reg);

  return "";
}

bool Bus::interrupt_pending() const { return io[IE] & io[IF]; }

bool Bus::should_raise_mode_0() {
  bool mode_0_interrupt_enabled = (io[STAT] & (1 << 3)) ? true : false;
  return mode_0_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::HBLANK;
};

bool Bus::should_raise_mode_1() {
  bool mode_1_interrupt_enabled = (io[STAT] & (1 << 4)) ? true : false;
  return mode_1_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::VBLANK;
};

bool Bus::should_raise_mode_2() {
  bool mode_2_interrupt_enabled = (io[STAT] & (1 << 5)) ? true : false;
  return mode_2_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::OAM_SCAN;
};

bool Bus::should_raise_ly_lyc() {
  bool ly_interrupt_enabled = (io[STAT] & (1 << 6)) ? true : false;  // LYC int select

  return ly_interrupt_enabled && (io[LYC] == io[LY]);
};