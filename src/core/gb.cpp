#include "core/gb.hpp"

#include <filesystem>

#include "bus.hpp"
#include "cpu.hpp"
#include "fmt/base.h"
#include "io_defs.hpp"
#include "mapper.hpp"

void GB::init_hw_regs(SYSTEM_MODE mode) {
  switch (mode) {
    case SYSTEM_MODE::DMG: {
      fmt::println("INIT DMG REGS");
      bus.io[JOYPAD]      = 0xCF;
      bus.io[SB]          = 0x00;
      bus.io[SC]          = 0x7E;
      bus.io[UNUSED_FF03] = 0xFF;
      bus.io[DIV]         = 0x1E;
      bus.io[TIMA]        = 0x00;
      bus.io[TMA]         = 0x00;
      bus.io[TAC]         = 0xF8;
      bus.io[IF]          = 0xE9;
      bus.io[LCDC]        = 0x91;
      bus.io[STAT]        = 0x85;
      bus.io[SCY]         = 0x00;
      bus.io[SCX]         = 0x00;
      bus.io[LY]          = 0x90;
      bus.io[LYC]         = 0x00;
      bus.io[DMA]         = 0xFF;
      bus.io[BGP]         = 0xFC;
      bus.io[OBP0]        = 0xFF;
      bus.io[OBP1]        = 0xFF;
      bus.io[WY]          = 0x00;
      bus.io[WX]          = 0x00;

      bus.io[KEY1]  = 0x7E;
      bus.io[VBK]   = 0xFE;
      bus.io[HDMA1] = 0xFF;
      bus.io[HDMA2] = 0xFF;
      bus.io[HDMA3] = 0xFF;
      bus.io[HDMA4] = 0xFF;
      bus.io[HDMA5] = 0xFF;
      bus.io[RP]    = 0xFF;
      bus.io[BCPS]  = 0xC0;
      bus.io[BCPD]  = 0xFF;
      bus.io[OCPS]  = 0xC1;
      bus.io[OCPD]  = 0x0D;
      bus.io[SVBK]  = 0xFF;
      bus.io[IE]    = 0x00;

      bus.io[NR10] = 0x80;
      bus.io[NR11] = 0xBF;
      bus.io[NR12] = 0xF3;
      bus.io[NR13] = 0xFF;
      bus.io[NR14] = 0xBF;
      bus.io[NR21] = 0x3F;
      bus.io[NR22] = 0x00;
      bus.io[NR23] = 0xFF;
      bus.io[NR24] = 0xBF;
      bus.io[NR30] = 0x7F;
      bus.io[NR31] = 0xFF;
      bus.io[NR32] = 0x9F;
      bus.io[NR33] = 0xFF;
      bus.io[NR34] = 0xBF;
      bus.io[NR41] = 0xFF;
      bus.io[NR42] = 0x00;
      bus.io[NR43] = 0x00;
      bus.io[NR44] = 0xBF;
      bus.io[NR50] = 0x77;
      bus.io[NR51] = 0xF3;
      bus.io[NR52] = 0xF1;

      cpu.AF = 0x01B0;
      cpu.BC = 0x0013;
      cpu.DE = 0x00D8;
      // bus.io[KEY0]        = 0x80;
      cpu.HL = 0x014D;
      break;
    }
    case SYSTEM_MODE::CGB: {
      bus.io[JOYPAD]      = 0xCF;
      bus.io[SB]          = 0x00;
      bus.io[SC]          = 0x7F;
      bus.io[UNUSED_FF03] = 0xFF;
      bus.io[DIV]         = 0x1E;
      bus.io[TIMA]        = 0x00;
      bus.io[TMA]         = 0x00;
      bus.io[TAC]         = 0xF8;
      bus.io[IF]          = 0xE1;

      bus.io[LCDC] = 0x91;
      bus.io[STAT] = 0x81;
      bus.io[SCY]  = 0x00;
      bus.io[SCX]  = 0x00;
      bus.io[LY]   = 0x90;
      bus.io[LYC]  = 0x00;
      bus.io[DMA]  = 0x00;
      bus.io[BGP]  = 0xFC;
      bus.io[OBP0] = 0x00;
      bus.io[OBP1] = 0x00;
      bus.io[WY]   = 0x00;
      bus.io[WX]   = 0x00;
      // bus.io[KEY0]        = 0x80;
      bus.io[KEY1]  = 0x7E;
      bus.io[VBK]   = 0xFE;
      bus.io[HDMA1] = 0xFF;
      bus.io[HDMA2] = 0xFF;
      bus.io[HDMA3] = 0xFF;
      bus.io[HDMA4] = 0xFF;
      // fmt::println("INIT CGB REGS");
      bus.io[HDMA5] = 0xFF;
      // fmt::println("[IHW] HDMA5: {}", bus.io[HDMA5]);
      bus.io[RP]   = 0xFF;
      bus.io[BCPS] = 0xC0;
      bus.io[BCPD] = 0xFF;
      bus.io[OCPS] = 0xC1;
      bus.io[OCPD] = 0xA4;
      bus.io[SVBK] = 0xF8;
      bus.io[IE]   = 0x00;

      cpu.AF = 0x1180;
      cpu.BC = 0x0000;
      cpu.DE = 0xFF56;
      cpu.HL = 0x000D;
      break;
    }
  }
  cpu.PC    = 0x0100;
  cpu.SP    = 0xFFFE;
  cpu.speed = SPEED::NORMAL;
  bus.reset();
}

GB::GB() {
  Mapper::bus = &bus;
  cpu.bus     = &bus;
  ppu.bus     = &bus;
  timer.bus   = &bus;

  bus.apu   = &apu;
  bus.ppu   = &ppu;
  bus.cart  = &cart;
  bus.timer = &timer;

  apu.bus = &bus;

  fmt::println("[0] bus ptr on apu: {}", fmt::ptr(bus.timer));
  fmt::println("[0] bus ptr on apu: {}", fmt::ptr(timer.bus));
}

GB::~GB() {
  if (cart.info.title.empty()) return;

  if (!(bus.mapper->id == 0x03 || bus.mapper->id == 0x06 || bus.mapper->id == 0x09 || bus.mapper->id == 0x0D || bus.mapper->id == 0x0F || bus.mapper->id == 0x10 || bus.mapper->id == 0x13 ||
        bus.mapper->id == 0x1B || bus.mapper->id == 0x1E || bus.mapper->id == 0x22 || bus.mapper->id == 0xFF)) {  // mapper ids with save compatiblity
    return;
  }

  save_game();
}

void GB::load_cart(const File &rom) {
  reset();
  cart.memory    = rom.data;
  cart.info.path = rom.path;

  std::fill(cart.ext_ram.begin(), cart.ext_ram.end(), 0);

  bus.io[KEY0] = cart.memory[0x143];

  // check rom compat mode -- set hw regs on init

  if (bus.io[KEY0] == static_cast<u8>(SYSTEM_MODE::CGB) || bus.io[KEY0] == 0x80) {
    bus.mode = SYSTEM_MODE::CGB;
  } else {
    bus.mode = SYSTEM_MODE::DMG;
  }

  init_hw_regs(bus.mode);

  cart.set_cart_info();
  cart.print_cart_info();
  Mapper *mapper_ptr = get_mapper_by_id(cart.info.mapper_id);
  bus.mapper         = mapper_ptr;
  ppu.mapper         = mapper_ptr;

  load_save_game();

  cpu.status = Umibozu::SM83::STATUS::ACTIVE;
}

void GB::save_game() {
  if (!std::filesystem::exists("saves")) {
    if (!std::filesystem::create_directory("saves")) {
      fmt::println("could not create save directory");
    }
  }

  std::ofstream save(fmt::format("saves/{}.sav", cart.info.title), std::ios::binary | std::ios::trunc);

  std::ostream_iterator<u8> output_iterator(save);
  std::copy(std::begin(cart.ext_ram), std::end(cart.ext_ram), output_iterator);

  save.close();
}

void GB::load_save_game() {
  std::string save_path = fmt::format("saves/{}.sav", cart.info.title);

  if (std::filesystem::exists(save_path)) {
    std::ifstream save(save_path, std::ios::binary);

    File save_file = read_file(save_path);
    u64 index      = 0;
    for (auto &byte : save_file.data) {
      cart.ext_ram.at(index++) = byte;
    }

    fmt::println("[GB] save loaded");
  }
}

void GB::system_loop() {
  while (active) {
    cpu.run_instruction();
  }
}



void GB::reset() {
  cpu        = {};
  cpu.status = Umibozu::SM83::STATUS::PAUSED;
  cpu.bus    = &bus;

  timer = {};

  ppu.lcdc.value   = 0x91;
  ppu.dots         = 0;
  ppu.frame_queued = false;
  ppu.frame_skip   = false;

  ppu.hdma_active                        = false;
  ppu.remaining_length                   = 0;
  ppu.stat_irq_fired_on_current_scanline = false;
  ppu.x_pos_offset                       = 0;
  ppu.window_current_y                   = 0;
  ppu.window_x_pos_offset                = 0;
  ppu.window_enabled                     = 0;
  ppu.window_line_count                  = 0;

  ppu.hdma_executed_on_scanline = false;
  ppu.ly_is_lyc_latch           = false;

  ppu.DMG_BGP = {};
  ppu.DMG_OBP = {};

  ppu.CGB_BGP = {};
  ppu.CGB_OBP = {};

  // resetting of IO is handled in init_hw_regs
  bus.reset();
}