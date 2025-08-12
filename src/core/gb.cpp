#include "core/gb.hpp"

#include <filesystem>

#include "bus.hpp"
#include "cpu.hpp"
#include "mapper.hpp"

void GB::init_hw_regs(SYSTEM_MODE mode) {
  switch (mode) {
    case SYSTEM_MODE::DMG: {
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
      bus.io[HDMA5] = 0xFF;
      bus.io[RP]    = 0xFF;
      bus.io[BCPS]  = 0xC0;
      bus.io[BCPD]  = 0xFF;
      bus.io[OCPS]  = 0xC1;
      bus.io[OCPD]  = 0xA4;
      bus.io[SVBK]  = 0xF8;
      bus.io[IE]    = 0x00;

      cpu.AF = 0x1180;
      cpu.BC = 0x0000;
      cpu.DE = 0xFF56;
      cpu.HL = 0x000D;
      break;
    }
  }
  cpu.PC = 0x0100;
  cpu.SP = 0xFFFE;
}

GB::GB() {
  Mapper::bus = &bus;
  bus.apu     = &apu;
  cpu.ppu     = &ppu;
  cpu.timer   = &timer;
  timer.apu = &apu;
  cpu.bus   = &bus;
  ppu.bus   = &bus;
  bus.ppu   = &ppu;
}

GB::~GB() {
  if (bus.cart.info.title.empty()) {
    return;
  }

  if (!(cpu.mapper->id == 0x03 || cpu.mapper->id == 0x06 || cpu.mapper->id == 0x09 || cpu.mapper->id == 0x0D || cpu.mapper->id == 0x0F || cpu.mapper->id == 0x10 || cpu.mapper->id == 0x13 ||
        cpu.mapper->id == 0x1B || cpu.mapper->id == 0x1E || cpu.mapper->id == 0x22 || cpu.mapper->id == 0xFF)) {  // mapper ids with save compatiblity
    return;
  }

  save_game();
}

void GB::load_cart(const File &rom) {
  bus.cart.memory    = rom.data;
  bus.cart.info.path = rom.path;

  std::fill(bus.cart.ext_ram.begin(), bus.cart.ext_ram.end(), 0);

  bus.io[KEY0] = bus.cart.memory[0x143];

  // check rom compat mode -- set hw regs on init

  if (bus.io[KEY0] == (u8)SYSTEM_MODE::CGB || bus.io[KEY0] == 0x80) {
    bus.mode = SYSTEM_MODE::CGB;
  } else {
    bus.mode = SYSTEM_MODE::DMG;
  }

  init_hw_regs(bus.mode);

  bus.cart.set_cart_info();
  bus.cart.print_cart_info();
  Mapper *mapper_ptr = get_mapper_by_id(bus.cart.info.mapper_id);
  cpu.mapper         = mapper_ptr;
  ppu.mapper         = mapper_ptr;

  load_save_game();

  cpu.status = Umibozu::SM83::STATUS::ACTIVE;
}

void GB::save_game() {
  if (!std::filesystem::exists("saves")) {
    std::filesystem::create_directory("saves");
  }

  std::ofstream save(fmt::format("saves/{}.sav", bus.cart.info.title), std::ios::binary | std::ios::trunc);

  std::ostream_iterator<u8> output_iterator(save);
  std::copy(std::begin(bus.cart.ext_ram), std::end(bus.cart.ext_ram), output_iterator);

  save.close();
}

void GB::load_save_game() {
  std::string save_path = fmt::format("saves/{}.sav", bus.cart.info.title);

  if (std::filesystem::exists(save_path)) {
    std::ifstream save(save_path, std::ios::binary);

    File save_file = read_file(save_path);
    u64 index      = 0;
    for (auto &byte : save_file.data) {
      bus.cart.ext_ram[index++] = byte;
    }

    fmt::println("[GB] save loaded");
  }
}

void GB::system_loop() {
  while (active) {
    cpu.run_instruction();
  }
}