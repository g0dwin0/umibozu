#include "core/bus.hpp"

#include <string>

#include "common.hpp"
#include "fmt/base.h"
#include "io_defs.hpp"
#include "ppu.hpp"

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

bool Bus::should_raise_mode_0() const {
  bool mode_0_interrupt_enabled = (io[STAT] & (1 << 3)) ? true : false;
  return mode_0_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::HBLANK;
};

bool Bus::should_raise_mode_1() const {
  bool mode_1_interrupt_enabled = (io[STAT] & (1 << 4)) ? true : false;
  return mode_1_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::VBLANK;
};

bool Bus::should_raise_mode_2() const {
  bool mode_2_interrupt_enabled = (io[STAT] & (1 << 5)) ? true : false;
  return mode_2_interrupt_enabled && (ppu->ppu_mode == RENDERING_MODE::OAM_SCAN);
};

bool Bus::should_raise_ly_lyc() const {
  bool ly_interrupt_enabled = (io[STAT] & (1 << 6)) ? true : false;  // LYC int select

  return ly_interrupt_enabled && (io[LYC] == io[LY]);
};

u8 Bus::io_read(const u16 address) {
  u8 io_addr = (address - 0xFF00);

  if ((address - 0xFF00) >= 0x30 && (address - 0xFF00) <= 0x3F) {
    // fmt::println("WAVE RAM READ");
    return wave_ram.at(address - 0xFF30);
  }

  if (io_addr >= 0x10 && io_addr <= 0x2F) {
    u8 result = apu->read(static_cast<IO_REG>(address - 0xFF00));
    // fmt::println("APU read from: {} -> {:#02X}", get_label(address), result);
    return result;
  }

  switch (address - 0xFF00) {
    case HDMA1:
    case HDMA2:
    case HDMA3:
    case HDMA4: {
      fmt::println("[R] HDMA read");
      break;
    }

    case JOYPAD: {
      switch ((io[JOYPAD] & 0x30) >> 4) {
        case 0x1: {
          return joypad.get_buttons();
        }
        case 0x2: {
          return joypad.get_dpad();
        }
        case 0x3: {
          return 0xF;
        }
        default: {
          fmt::println("[JOYPAD] bad method bits");
          assert(false);
        }
      }

      break;
    }
    case DIV: {
      return timer->get_div();
    }
    case TIMA: {
      return timer->counter;
    }
    case TMA: {
      return timer->modulo;
    }
    case LY: {
      if (ppu->lcdc.lcd_ppu_enable == 0) {
        // fmt::println(("reading LY when screen off"));
        return 0;
      }
      return io[LY];
    }

    case LCDC: {
      return ppu->lcdc.value;
    }

    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F: {  // Audio has the range 0xFF10 to 0xFF30 however everything after
                  // FF26 is not mapped to anything.
      return 0xFF;
    }
    case STAT: {
      if (ppu->lcdc.lcd_ppu_enable == 0) {
        fmt::println("PPU is off, returning 0 in read");
        io[LY] = 0;
        return 0 | (1 << 7);
      }
      break;
    }
    case SVBK: {
      // fmt::println("SVBK: {:#04x}", io[SVBK]);
      break;
    }
    case VBK: {
      // fmt::println("VBK: {:#04x}", io[VBK]);
      break;
    }
    case KEY1: {
      if (mode == SYSTEM_MODE::DMG) return 0xFF;

      u8 speed = double_speed_mode ? 0x80 : 0;

      // fmt::println("returning key1: {:08b}", (static_cast<u8>(speed)) | (io[KEY1] & 1));
      return (static_cast<u8>(speed)) | (io[KEY1] & 1);
    }
    case HDMA5: {
      fmt::println("HDMA5 read: {:08b}", io[HDMA5]);
      break;
    }
  }

  return io.at(address - 0xFF00);
}

void Bus::io_write(const u16 address, const u8 value) {
  // fmt::println("[W] {:010X} - {}", address, get_label(address));
  u8 io_addr = (address - 0xFF00);

  // wave ram
  if (io_addr >= 0x30 && io_addr <= 0x3F) {
    // fmt::println("WAVE RAM WRITE");
    wave_ram.at(io_addr - 0x30) = value;
    return;
  }

  // apu registers
  if (io_addr >= 0x10 && io_addr <= 0x2F) {
    // fmt::println("APU write to: {:#04X} [{}] -> {:#02X}", address, get_label(address), value);
    return apu->write(static_cast<IO_REG>(io_addr), value);
  }

  switch (io_addr) {
    case JOYPAD: {
      if (value == 0x30) {  // BUTTONS NOR D-PAD SELECTED
        io[JOYPAD] = 0xFF;
        return;
      }
      break;
    }
    case SC: {
      // DEPRECATED: doesn't have much use except for logging; remove when color
      // is implemented & working
      if (value == 0x81) {
        // serial_port_buffer[serial_port_index++] =
        // wram[SB]; std::string str_data(serial_port_buffer,
        // SERIAL_PORT_BUFFER_SIZE); fmt::println("serial data: {}", str_data);
      }
      if (value == 0x01) {
        fmt::println("transfer completed");
        request_interrupt(INTERRUPT_TYPE::SERIAL);
      }
      break;
    }
    case DIV: {
      timer->reset_div(double_speed_mode);
      break;
    }
    case TIMA: {
      if (timer->overflow_update_queued) {
        timer->overflow_update_queued = false;
      }
      timer->counter = value;

      break;
    }
    case TMA: {
      timer->modulo = value;
      break;
    }
    case TAC: {
      timer->set_tac(value);
      break;
    }
    case LCDC: {
      // if ((io[LCDC] & 0x80) == 0 && (value & 0x80)) {  // PPU goes from off to on
      //   io[LY] = 0;
      //   ppu->dots   = 4;
      // }
      // if ((io[LCDC] & 0x80) && (value & 0x80) == 0) {
      //   // LCDC turned off, latch LY=LYC
      //   ppu->ly_is_lyc_latch = io[LY] == io[LYC];
      // }
      ppu->lcdc.value = value;
      break;
    }
    case STAT: {
      fmt::println("STAT: {:08b} - {:#04X}", value, value);
      u8 read_only_bits = value & 0x78;

      bool old_hidden_stat = hidden_stat;

      io[STAT] = read_only_bits + (u8)ppu->get_mode();

      update_hidden_stat();

      if (old_hidden_stat == 0 && hidden_stat == 1) {
        request_interrupt(INTERRUPT_TYPE::LCD);
      }

      return;
    }
    case LY: {  // read only
      return;
    }
    case LYC: {
      bool old_hidden_stat = hidden_stat;

      io[LYC] = value;

      update_hidden_stat();

      if (old_hidden_stat == 0 && hidden_stat == 1) {
        request_interrupt(INTERRUPT_TYPE::LCD);
      }

      break;
    }
    case DMA: {
      u16 address = (value << 8);
      // fmt::println("executing DMA");
      for (size_t i = 0; i < 0xA0; i++) {
        oam.at(i) = read8(address + i);
      }
      break;
    }
    case BGP: {
      // if (mode == SYSTEM_MODE::CGB) {
      //   return;
      // }
      // fmt::println("OBP0 write: {:08b}", value);
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->DMG_BGP[0][0] = ppu->shade_table[id_0];
      ppu->DMG_BGP[0][1] = ppu->shade_table[id_1];
      ppu->DMG_BGP[0][2] = ppu->shade_table[id_2];
      ppu->DMG_BGP[0][3] = ppu->shade_table[id_3];
      break;
    }
    case OBP0: {
      // if (mode == SYSTEM_MODE::CGB) {
      //   return;
      // }
      // fmt::println("OBP0 write: {:08b}", value);
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->DMG_OBP[0][0] = ppu->shade_table[id_0];
      ppu->DMG_OBP[0][1] = ppu->shade_table[id_1];
      ppu->DMG_OBP[0][2] = ppu->shade_table[id_2];
      ppu->DMG_OBP[0][3] = ppu->shade_table[id_3];
      break;
    }
    case OBP1: {
      // if (mode == SYSTEM_MODE::CGB) {
      //   return;
      // }
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->DMG_OBP[1][0] = ppu->shade_table[id_0];
      ppu->DMG_OBP[1][1] = ppu->shade_table[id_1];
      ppu->DMG_OBP[1][2] = ppu->shade_table[id_2];
      ppu->DMG_OBP[1][3] = ppu->shade_table[id_3];
      break;
    }
    // GBC-only IO
    case VBK: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      vbk  = (value & 0x1);
      vram = &vram_banks.at(vbk);

      io[VBK] = 0xFE + vbk;
      return;
    }
    case SVBK: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      svbk = (value & 0x7);
      if (svbk == 0) {
        svbk = 1;
      }

      wram = &wram_banks.at(svbk);

      io[SVBK] = 0xF8 + svbk;
      return;
    }
    case KEY1: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      io[KEY1] |= 1;

      return;
    }
    case HDMA1: {
      fmt::println("HDMA1: {:#010x}", value);
      break;
    }
    case HDMA2: {
      fmt::println("HDMA2: {:#010x}", value);
      break;
    }
    case HDMA3: {
      fmt::println("HDMA3: {:#010x}", value);
      break;
    }
    case HDMA4: {
      fmt::println("HDMA4: {:#010x}", value);
      break;
    }
    case HDMA5: {
      if (mode != SYSTEM_MODE::CGB) return;
      fmt::println("HDMA VALUE PRE-WRITE: {:08b}", io[HDMA5]);
      fmt::println("writing HDMA5 -- {:08b}", value);

      // fmt::println("[(G/H)DMA] initiatied with value: {:#08b}", value);
      bool dma_is_active    = (io[HDMA5] & 0x80) == 0;  // hdma is active
      bool cancel_requested = (value & 0x80) == 0;

      if (dma_is_active && cancel_requested) {
        fmt::println("[HDMA] terminated HDMA early");
        io[HDMA5] = value;
        terminate_hdma();
        return;
      }

      bool is_hdma = ((value & (1 << 7)) != 0);

      u16 length = (1 + (value & 0x7f)) * 0x10;
      u16 src    = ((io[HDMA1] << 8) + io[HDMA2]) & 0xfff0;
      u16 dst    = ((io[HDMA3] << 8) + io[HDMA4]) & 0x1ff0;

      fmt::println("[{}DMA] src:     {:#16x}", is_hdma ? "H" : "G", src);
      fmt::println("[{}DMA] dst:     {:#16x} ({:#04x})", is_hdma ? "H" : "G", dst, VRAM_ADDRESS_OFFSET + dst);
      fmt::println("[{}DMA] length:  {:#16x}", is_hdma ? "H" : "G", length);

      if (!is_hdma) {  // GDMA
        for (size_t index = 0; index < length; index++) {
          u16 src_address = (src + index);
          u8 src_data     = mapper->read8(src_address);

          u16 vram_address = ((dst + index));

          if (vram_address > 0x1FFF) {
            // fmt::println("[GDMA] skipped GDMA");
            io[HDMA5] = 0xFF;
            exit(-1);
            return;
          }
          fmt::println("[GDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: {:#16x} ", src_address, src_data, 0x8000 + vram_address);

          vram->at(vram_address) = src_data;
        }
        fmt::println("[GDMA] complete");
        src += length;
        dst += length;

        io[HDMA1] = (src >> 8);
        io[HDMA2] = (src & 0xFF);
        io[HDMA3] = (dst >> 8);
        io[HDMA4] = (dst & 0xF0);
        io[HDMA5] = 0xFF;
        return;
      } else {  // hblank dma
        init_hdma(length);

        return;
      }
      break;
    }

    case BCPS: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      bcps.v = value;

      break;
    }
    case BCPD: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }
      bg_palette_ram[bcps.address] = value;

      if (bcps.auto_increment) {
        bcps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low  = bg_palette_ram.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high = (bg_palette_ram.at((palette_id * 8) + (color * 2) + 1));

          u16 f_color = (p_high << 8) + p_low;

          ppu->CGB_BGP[palette_id][color] = f_color;
        }
      }
      break;
    }
    case OCPS: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }
      ocps.v = value;
      break;
    }
    case OCPD: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      obj_palette_ram.at(ocps.address) = value;
      if (ocps.auto_increment) {
        ocps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low  = obj_palette_ram.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high = (obj_palette_ram.at((palette_id * 8) + (color * 2) + 1));

          ppu->CGB_OBP[palette_id][color] = (p_high << 8) + p_low;
        }
      }
      break;
    }
    case IE: {
      // fmt::println("new IE: {}", value);
      break;
    }
  }

  io.at(address - 0xFF00) = value;
};

void Bus::init_hdma(u16 length) {
  io[HDMA5] = (length / 0x10) - 1;
  fmt::println("HDMA initialized, remaining blocks: {:#010x}", io[HDMA5]);
  // if (ppu->ppu_mode == RENDERING_MODE::HBLANK) ppu->process_hdma_chunk();
};
void Bus::terminate_hdma() {
  // fmt::println("HDMA terminated early.");
  io[HDMA5] |= (1 << 7);

  // fmt::println("HDMA5: {:08b}", io[HDMA5]);
};

u8 Bus::read8(const u16 address) {
  assert(cart != nullptr);
  assert(vram != nullptr);
  assert(wram != nullptr);

  if (address <= 0x3FFF) {
    return cart->read8(address);
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    return mapper->read8(address);
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    return vram->at((address - 0x8000));
  }
  if (address >= 0xA000 && address <= 0xBFFF) {
    return mapper->read8(address);
  }
  if (address >= 0xC000 && address <= 0xCFFF) {
    return wram_banks[0].at(address - 0xC000);
  }
  if (address >= 0xD000 && address <= 0xDFFF) {
    return wram->at((address - 0xD000));
  }

  if (address >= 0xE000 && address <= 0xFDFF) {
    return read8((address & 0xDFFF));
  }
  if (address >= 0xFE00 && address <= 0xFE9F) {
    return oam.at(address - 0xFE00);
  }

  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return 0x00;
  }

  if ((address >= 0xFF00 && address <= 0xFF7F) || address == 0xFFFF) {
    return io_read(address);
  }

  if (address >= 0xFF80 && address <= 0xFFFE) {
    return hram.at(address - 0xFF80);
  }
  if (address == 0xFFFF) {
    return io.at(IE);
  }

  throw std::runtime_error(fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}

void Bus::write8(const u16 address, const u8 value) {
  if (address <= 0x7FFF) {
    mapper->write8(address, value);
    return;
  }

  if (address >= 0x8000 && address <= 0x9FFF) {
    vram->at(address - 0x8000) = value;
    return;
  }

  if (address >= 0xA000 && address <= 0xBFFF) {
    mapper->write8(address, value);
    return;
  }

  if (address >= 0xC000 && address <= 0xCFFF) {
    wram_banks[0].at(address - 0xC000) = value;
    return;
  }

  if (address >= 0xD000 && address <= 0xDFFF) {
    wram->at(address - 0xD000) = value;
    return;
  }

  if (address >= 0xE000 && address <= 0xFDFF) {
    write8((address & 0xDFFF), value);
    return;
  }

  if (address >= 0xFE00 && address <= 0xFE9F) {
    oam.at(address - 0xFE00) = value;
    return;
  }

  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return;
  }

  if ((address >= 0xFF00 && address <= 0xFF7F) || address == 0xFFFF) {
    // fmt::println("io write");
    // fmt::println("write requested [{}] {:#04x} -> {:#02X}", bus->get_label(address), address, value);
    return io_write(address, value);
  };

  if (address >= 0xFF80 && address <= 0xFFFE) {
    hram.at(address - 0xFF80) = value;
    return;
  }

  fmt::println("address: {:#010x}", address);
  assert(0);
}

void Bus::reset() {
  vram_banks = {};
  wram_banks = {};

  vram = &vram_banks[0];
  wram = &wram_banks[1];

  bcps = {};
  ocps = {};

  oam = {};
  // io              = {};
  hram            = {};
  bg_palette_ram  = {};
  obj_palette_ram = {};

  hidden_stat = {};

  svbk = 0;
  vbk  = 0;

  fmt::println("[1] bus ptr on apu: {}", fmt::ptr(timer));
  timer->bus = this;
  fmt::println("[1] bus ptr on apu: {}", fmt::ptr(timer->bus));
}