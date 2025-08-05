#include "core/bus.hpp"

#include "common.hpp"
#include "enums.hpp"


void Bus::request_interrupt(InterruptType t) { io[IF] |= (1 << (u8)t); }

std::string Bus::get_mode_string() const {
  if (mode == SYSTEM_MODE::CGB) {
    return "CGB";
  } else {
    return "DMG";
  }
}

bool Bus::interrupt_pending() const { return io[IE] & io[IF]; }

u8 Bus::io_read(const u16 address) {
  if (address != (0xFF00 + SB) && address != (0xFF00 + SC) && address != (0xFF00 + LY) && address != (0xFF00 + LCDC) && address != (0xFF00 + JOYPAD)) {
    // ("[R] register address: {:#04x}, value: {:#04x}", address,
    //              io[address - 0xFF00]);
  }

  switch (address - 0xFF00) {
    case 0x30 ... 0x3F: {  // Wave RAM
      // ("[APU] reading from Wave RAM: {:#16x} - {:#08x}", address,
      //  wave_ram.read8((address - 0xFF30)));
      return wave_ram.at((address - 0xFF30));
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
          io_logger->info("[JOYPAD] bad method bits");
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
      break;
    }

    // case NR10 ... NR52: {
    //   u8 retval = 0;
    //   retval    = apu->handle_read((IO_REG)(address - 0xFF00));
    //   // ("A: {}", A);
    //   // ("B: {}", B);
    //   // ("[APU] reading {:#4x} from {:#4x}", retval, (address));
    //   return retval;
    //   // break;
    // }
    case 0x27 ... 0x2f: {  // Audio is 0xFF10 to 0xFF30 however everything after
                           // FF26 is not mapped to any register.
      return 0xFF;
    }
    case STAT: {
      if (ppu->lcdc.lcd_ppu_enable == 0) {
        return 0;
      }
      break;
    }
    case SVBK: {
      io_logger->info("SVBK: {:#04x}", io[SVBK]);
      break;
    }
    case VBK: {
      io_logger->info("VBK: {:#04x}", io[VBK]);
      break;
    }
    case KEY1: {
      io_logger->info("returning key1: {:#04x}", 0x00);
      return 0;  // TODO: return valid whatever
    }
    case HDMA5: {
      io_logger->info("HDMA5 read while active: {:08b}", io[HDMA5]);
      break;
    }
  }
  return io.at(address - 0xFF00);
}

u8 Bus::read8(const u16 address) {
  #ifdef CPU_TEST_MODE
  return test_memory[address];
  #endif
  assert(mapper != nullptr);
  m_cycle();
  // ("[R] {:#04x}", address);
  if (address >= 0xFE00 && address <= 0xFE9F && (u8)ppu->get_mode() >= 2) {
    return 0xFF;
  }

  if ((address >= 0xFF00 && address <= 0xFF7F) || address == 0xFFFF) {
    // ("io write");
    return io_read(address);
  };
  return mapper->read8(address);
};

u16 Bus::read16(const u16 address) {
  u8 low  = read8(address);
  u8 high = read8(address + 1);
  // PC++;
  return (high << 8) + low;
}
// debug only!
u8 Bus::peek(const u16 address) const { return mapper->read8(address); }

// void SM83::init_hdma(u16 length) {
//   ppu->hdma_index = 0;
//   io[HDMA5]  = (length / 0x10) - 1;
// };
// void SM83::terminate_hdma() {
//   ("HDMA terminated early.");
//   io[HDMA5] &= ~(1 << 7);

//   ("HDMA5: {:08b}", io[HDMA5]);
// };

void Bus::io_write(const u16 address, const u8 value) {
  if (address != (0xFF00 + SB) && address != (0xFF00 + SC) && address != (0xFF00 + LY) && address != (0xFF00 + LCDC) && address != (0xFF00 + JOYPAD)) {
    // ("[W] register address: {:#04x}, value: {:#04x}", address,
    //              value);
  }
  switch (address - 0xFF00) {
    case 0x30 ... 0x3F: {  // Wave RAM
      // ("[APU] writing to Wave RAM: {:#04x} - {:#04x}", address,
      //  value);
      wave_ram.at((address - 0xFF30)) = value;
      return;
    }
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
        // wram.data[SB]; std::string str_data(serial_port_buffer,
        // SERIAL_PORT_BUFFER_SIZE); ("serial data: {}", str_data);
      }
      if (value == 0x01) {
        io_logger->info("transfer completed");
        request_interrupt(InterruptType::SERIAL);
      }
      break;
    }
    case DIV: {
      timer->reset_div();
      break;
    }
    case TIMA: {
      timer->counter = value;
      break;
    }
    case TMA: {
      timer->modulo = value;
      break;
    }
    case TAC: {
      timer->set_tac(value);
      // ("TAC: {:08b}", value);
      break;
    }
    case LCDC: {
      if ((io[LCDC] & 0x80) == 0 && (value & 0x80)) {
        ppu->frame_skip = true;
        ppu->frame.clear();
        io[LY] = 0;
      }
      ppu->lcdc.value = value;
      break;
    }
    case STAT: {
      io_logger->info("STAT: {:08b}", value);
      u8 read_only_bits = value & 0x78;
      io[STAT]     = (1 << 7) + read_only_bits + (u8)ppu->get_mode();
      return;
    }
    case LY: {  // read only
      return;
    }
    case LYC: {
      io[LYC] = value;
      io[STAT] &= 0xfb;
      io[STAT] |= (io[LY] == io[LYC]) ? (1 << 2) : 0;

      if ((io[STAT] & 1 << 6) && (io[STAT] & 1 << 2)) {
        request_interrupt(InterruptType::LCD);
      }

      break;
    }
    case DMA: {
      u16 address = (value << 8);
      for (size_t i = 0; i < 0xA0; i++) {
        oam[i] = read8(address + i);
      }
      break;
    }
    case BGP: {
      // ("OBP0 write: {:08b}", value);
      u8 id_0 = 0;
      u8 id_1 = (value & 0b00001100) >> 2;
      u8 id_2 = (value & 0b00110000) >> 4;
      u8 id_3 = (value & 0b11000000) >> 6;

      ppu->sys_palettes.BGP[0][0] = ppu->shade_table[id_0];
      ppu->sys_palettes.BGP[0][1] = ppu->shade_table[id_1];
      ppu->sys_palettes.BGP[0][2] = ppu->shade_table[id_2];
      ppu->sys_palettes.BGP[0][3] = ppu->shade_table[id_3];

      //      ("BGP0[0] = {:#16x}", ppu->sys_palettes.BGP[0][0]);
      //      ("BGP0[1] = {:#16x}", ppu->sys_palettes.BGP[0][1]);
      //      ("BGP0[2] = {:#16x}", ppu->sys_palettes.BGP[0][2]);
      //      ("BGP0[3] = {:#16x}", ppu->sys_palettes.BGP[0][3]);
      break;
    }
    case OBP0: {
      // ("OBP0 write: {:08b}", value);
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

    // case NR10 ... NR52: {
    //   apu->handle_write(value, (IO_REG)(address - 0xFF00));

    //   break;
    // }

    // GBC-only IO
    case VBK: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      vbk  = (value & 0x1);
      vram = &vram_banks[vbk];

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

      wram = &wram_banks[svbk];

      io[SVBK] = 0xF8 + svbk;
      return;
    }
    case KEY1: {
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }

      io_logger->info("[KEY1] value: {:#04x}", value);
      io_logger->info("[KEY1] desired speed: {:#04x}", value & 0x80);
      io_logger->info("[KEY1] current speed: {:#04x}", io[KEY1] & 0x80);

      u8 current_speed = (io[KEY1] & 0x80);
      u8 desired_speed = (value & 0x80);

      if (current_speed != desired_speed) {
        io[KEY1] = desired_speed;
        // speed         = (SPEED)desired_speed;
        timer->reset_div();
        // timer->div          = 0; // TODO: make a reset function
        io_logger->info("[KEY1] speed switched");
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
      if (mode != SYSTEM_MODE::CGB) {
        return;
      }
      io_logger->info("[HDMA] entered HDMA5, value: {:#08b}", value);

      // if ((io[HDMA5] & 0x80) == 0 && (value & 0x80) == 0x80) {
      //   io_logger->info("[HDMA] terminated HDMA");

      //   return terminate_hdma();
      // }

      // u16 length = (1 + (value & 0x7f)) * 0x10;
      // u16 src    = ((io[HDMA1] << 8) + io[HDMA2]) & 0xfff0;
      // u16 dst    = ((io[HDMA3] << 8) + io[HDMA4]) & 0x1ff0;

      // io_logger->info("[HDMA] src:     {:#16x}", src);
      // io_logger->info("[HDMA] dst:     {:#16x} ({:#04x})", dst, VRAM_ADDRESS_OFFSET + dst);
      // io_logger->info("[HDMA] length:  {:#16x}", length);

      // if ((value & 0x80) == 0) {  // GDMA
      //   for (size_t index = 0; index < length; index++) {
      //     u16 src_address = (src + index);
      //     u8 src_data     = mapper->read8(src_address);

      //     u16 vram_address = ((dst + index));

      //     if (vram_address > 0x1FFF) {
      //       io_logger->info("[GDMA] skipped GDMA");
      //       io[HDMA5] = 0xFF;
      //       exit(-1);
      //       return;
      //     }

      //     // (
      //     // "[GDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: {:#16x} ",
      //     // src_address, src_data, 0x8000 + vram_address);

      //     vram->write8(vram_address, src_data);
      //   }
      //   io[HDMA5] = 0xFF;
      //   return;
      // } else {  // hblank dma
      //   if ((io[HDMA5] & 0x80) == 0x80) {
      //     init_hdma(length);
      //   }
      //   return;
      // }
      break;
    }

    case BCPS: {
      if (mode != SYSTEM_MODE::CGB) return;

      bcps.address        = value & 0b111111;
      bcps.auto_increment = (value & (1 << 7)) >> 7;

      break;
    }
    case BCPD: {
      if (mode != SYSTEM_MODE::CGB) return;

      bg_palette_ram[bcps.address] = value;
      if (bcps.auto_increment) {
        bcps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low  = bg_palette_ram.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high = (bg_palette_ram.at((palette_id * 8) + (color * 2) + 1));

          u16 f_color = (p_high << 8) + p_low;

          ppu->sys_palettes.BGP[palette_id][color] = f_color;
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

      obj_palette_ram.at(ocps.address) = value;
      if (ocps.auto_increment) {
        ocps.address++;
      }

      for (size_t palette_id = 0; palette_id < 8; palette_id++) {
        for (size_t color = 0; color < 4; color++) {
          u8 p_low  = obj_palette_ram.at((palette_id * 8) + (color * 2) + 0);
          u8 p_high = (obj_palette_ram.at((palette_id * 8) + (color * 2) + 1));

          ppu->sys_palettes.OBP[palette_id][color] = (p_high << 8) + p_low;
        }
      }
      break;
    }
  }




  // return io.write8(address - 0xFF00, value);
};

void Bus::write8(const u16 address, const u8 value) {
#ifdef CPU_TEST_MODE
  test_memory[address] = value;
  return;
#endif
  m_cycle();
  switch (address) {
    case 0xFE00 ... 0xFE9F: {
      if (ppu->get_mode() >= 2) return;
      break;
    }

    case 0xFF00 ... 0xFF7F:
    case 0xFFFF: {
      io_write(address, value);
      break;
    };

    default: {  // defer to the mapper
      mapper->write8(address, value);
    }
  }

  return;
}

void Bus::m_cycle() {
#ifndef CPU_TEST_MODE
  // if (speed == SPEED::DOUBLE) {
  //   timer->increment_div(2);
  //   // mapper->increment_internal_clock(2, mapper->actual.RTC_DAY);
  //   ppu->tick(2);
  // } else {
    timer->increment_div(4);
    // mapper->increment_internal_clock(4, mapper->actual.RTC_DAY); // refactor: this is not elegant
    ppu->tick(4);
  // }

  if (timer->overflow_update_queued) {
    timer->overflow_update_queued = false;
    timer->counter                = timer->modulo;
    request_interrupt(InterruptType::TIMER);
  }

  if (timer->ticking_enabled) {
    // u16 div_bits = (timer->div & (1 << TIMER_BIT[io[TAC] & 0x3]))
    // >>
    //                TIMER_BIT[io[TAC] & 0x3];

    u16 div_bits = (timer->get_full_div() & (1 << TIMER_BIT[io[TAC] & 0x3])) >> TIMER_BIT[io[TAC] & 0x3];

    u8 te_bit = (io[TAC] & (1 << 2)) >> 2;

    u8 n_val = div_bits & te_bit;

    if (n_val == 0 && timer->prev_and_result == 1) {
      if (timer->counter == 0xFF) {
        timer->overflow_update_queued = true;
      }
      timer->counter++;
    }
    timer->prev_and_result = n_val;
  }

#endif
}