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
  return mode_2_interrupt_enabled && ppu->ppu_mode == RENDERING_MODE::OAM_SCAN;
};

bool Bus::should_raise_ly_lyc() const {
  bool ly_interrupt_enabled = (io[STAT] & (1 << 6)) ? true : false;  // LYC int select

  return ly_interrupt_enabled && (io[LYC] == io[LY]);
};

u8 Bus::io_read(const u16 address) {
  // if ((address - 0xFF00) >= 0x30 && 0x3F <= (address - 0xFF00)) {
  //   // fmt::println("[APU] reading from Wave RAM: {:#16x} - {:#08x}", address, wave_ram.at((address - 0xFF30)));
  //   return wave_ram.at((address - 0xFF30));
  // }

  switch (address - 0xFF00) {
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
        return 0;
      }
      return io[LY];
    }

    case LCDC: {
      // break;
      return ppu->lcdc.value;
    }
    case NR10:
    case NR11:
    case NR12:
    case NR13:
    case NR14:
    case NR21:
    case NR22:
    case NR23:
    case NR24:
    case NR30:
    case NR31:
    case NR32:
    case NR33:
    case NR34:
    case NR41:
    case NR42:
    case NR43:
    case NR44:
    case NR50:
    case NR51:
    case NR52: {
      u8 retval = 0;
      retval    = apu->read((IO_REG)(address - 0xFF00));
      // fmt::println("[APU] reading {:#4x} from {:#4x}", retval, (address));
      return retval;
      // break;
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

      fmt::println("returning key1: {:08b}", (static_cast<u8>(speed)) | (io[KEY1] & 1));
      return (static_cast<u8>(speed)) | (io[KEY1] & 1);
    }
    case HDMA5: {
      fmt::println("HDMA5 read while active: {:08b}", io[HDMA5]);
      break;
    }
  }

  return io.at(address - 0xFF00);
}

void Bus::io_write(const u16 address, const u8 value) {
  u8 io_addr = (address - 0xFF00);

  // if (io_addr >= NR10 && NR52 <= io_addr) { // APU writes ignored
  //   apu->write(value, (IO_REG)io_addr);
  //   return;
  // }

  // if (io_addr >= 0x30 && 0x3F <= io_addr) {  // WAVE RAM
  //   wave_ram.at(address - 0xFF30) = value;
  //   return;
  // }

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
      timer->reset_div();
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
      fmt::println("STAT: {:08b}", value);
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

      //      fmt::println("BGP0[0] = {:#16x}", ppu->sys_palettes.BGP[0][0]);
      //      fmt::println("BGP0[1] = {:#16x}", ppu->sys_palettes.BGP[0][1]);
      //      fmt::println("BGP0[2] = {:#16x}", ppu->sys_palettes.BGP[0][2]);
      //      fmt::println("BGP0[3] = {:#16x}", ppu->sys_palettes.BGP[0][3]);
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

      fmt::println("write: {}", value);

      // u8 current_speed = (io[KEY1] & 0x80);
      // u8 desired_speed = (value & 0x80);

      // if (current_speed != desired_speed) {
      fmt::println("[CPU] value: {:#04x}", value);
      fmt::println("[CPU] desired speed: {:#04x}", value & 0x80);
      fmt::println("[CPU] current speed: {:#04x}", io[KEY1] & 0x80);

      // requested_speed = static_cast<SPEED>(desired_speed);
      io[KEY1] |= 1;

      fmt::println("[CPU] speed switch armed");
      // }

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

      // fmt::println("[(G/H)DMA] initiatied with value: {:#08b}", value);

      if ((io[HDMA5] & 0x80) == 0 && (value & 0x80) == 0x80) {
        // fmt::println("[HDMA] terminated HDMA");
        return terminate_hdma();
      }

      bool is_hdma = ((value & 0x80) != 0);

      u16 length = (1 + (value & 0x7f)) * 0x10;
      u16 src    = ((io[HDMA1] << 8) + io[HDMA2]) & 0xfff0;
      u16 dst    = ((io[HDMA3] << 8) + io[HDMA4]) & 0x1ff0;

      // fmt::println("[{}DMA] src:     {:#16x}", is_hdma ? "H" : "G", src);
      // fmt::println("[{}DMA] dst:     {:#16x} ({:#04x})", is_hdma ? "H" : "G", dst, VRAM_ADDRESS_OFFSET + dst);
      // fmt::println("[{}DMA] length:  {:#16x}", is_hdma ? "H" : "G", length);

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
          // fmt::println("[GDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: {:#16x} ", src_address, src_data, 0x8000 + vram_address);

          vram->at(vram_address) = src_data;
        }
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

      bcps.address        = value & 0b111111;
      bcps.auto_increment = (value & (1 << 7)) >> 7;

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
      ocps.address        = value & 0x3f;
      ocps.auto_increment = (value & (1 << 7)) >> 7;

      break;
    }
    case OCPD: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      obj_palette_ram[ocps.address] = value;
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
  ppu->hdma_index = 0;

  io[HDMA5] = (length / 0x10) - 1;
  fmt::println("HDMA initialized, remaining blocks: {:#010x}", io[HDMA5]);
};
void Bus::terminate_hdma() {
  fmt::println("HDMA terminated early.");
  io[HDMA5] &= ~(1 << 7);

  // fmt::println("HDMA5: {:08b}", io[HDMA5]);
};

u8 Bus::read8(const u16 address) {
  if (address <= 0x3FFF) {
    return cart->read8(address);
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    return vram->at((address - 0x8000));
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
    return io.at(address - 0xFF00);
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
