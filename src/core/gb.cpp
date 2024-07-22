#include "core/gb.h"

#include <filesystem>

#include "bus.h"
#include "cpu.h"
#include "mapper.h"

void GB::init_hw_regs(SYSTEM_MODE mode) {
  switch (mode) {
    case SYSTEM_MODE::DMG: {
      bus.io.data[JOYPAD]      = 0xCF;
      bus.io.data[SB]          = 0x00;
      bus.io.data[SC]          = 0x7E;
      bus.io.data[UNUSED_FF03] = 0xFF;
      bus.io.data[DIV]         = 0x1E;
      bus.io.data[TIMA]        = 0x00;
      bus.io.data[TMA]         = 0x00;
      bus.io.data[TAC]         = 0xF8;
      bus.io.data[IF]          = 0xE9;
      bus.io.data[LCDC]        = 0x91;
      bus.io.data[STAT]        = 0x85;
      bus.io.data[SCY]         = 0x00;
      bus.io.data[SCX]         = 0x00;
      bus.io.data[LY]          = 0x90;
      bus.io.data[LYC]         = 0x00;
      bus.io.data[DMA]         = 0xFF;
      bus.io.data[BGP]         = 0xFC;
      bus.io.data[OBP0]        = 0xFF;
      bus.io.data[OBP1]        = 0xFF;
      bus.io.data[WY]          = 0x00;
      bus.io.data[WX]          = 0x00;

      bus.io.data[KEY1]  = 0x7E;
      bus.io.data[VBK]   = 0xFE;
      bus.io.data[HDMA1] = 0xFF;
      bus.io.data[HDMA2] = 0xFF;
      bus.io.data[HDMA3] = 0xFF;
      bus.io.data[HDMA4] = 0xFF;
      bus.io.data[HDMA5] = 0xFF;
      bus.io.data[RP]    = 0xFF;
      bus.io.data[BCPS]  = 0xC0;
      bus.io.data[BCPD]  = 0xFF;
      bus.io.data[OCPS]  = 0xC1;
      bus.io.data[OCPD]  = 0x0D;
      bus.io.data[SVBK]  = 0xFF;
      bus.io.data[IE]    = 0x00;

      cpu.AF = 0x01B0;
      cpu.BC = 0x0013;
      cpu.DE = 0x00D8;
      // bus.io.data[KEY0]        = 0x80;
      cpu.HL = 0x014D;
      break;
    }
    case SYSTEM_MODE::CGB: {
      bus.io.data[JOYPAD]      = 0xCF;
      bus.io.data[SB]          = 0x00;
      bus.io.data[SC]          = 0x7F;
      bus.io.data[UNUSED_FF03] = 0xFF;
      bus.io.data[DIV]         = 0x1E;
      bus.io.data[TIMA]        = 0x00;
      bus.io.data[TMA]         = 0x00;
      bus.io.data[TAC]         = 0xF8;
      bus.io.data[IF]          = 0xE1;

      bus.io.data[LCDC] = 0x91;
      bus.io.data[STAT] = 0x81;
      bus.io.data[SCY]  = 0x00;
      bus.io.data[SCX]  = 0x00;
      bus.io.data[LY]   = 0x90;
      bus.io.data[LYC]  = 0x00;
      bus.io.data[DMA]  = 0x00;
      bus.io.data[BGP]  = 0xFC;
      bus.io.data[OBP0] = 0x00;
      bus.io.data[OBP1] = 0x00;
      bus.io.data[WY]   = 0x00;
      bus.io.data[WX]   = 0x00;
      // bus.io.data[KEY0]        = 0x80;
      bus.io.data[KEY1]  = 0x7E;
      bus.io.data[VBK]   = 0xFE;
      bus.io.data[HDMA1] = 0xFF;
      bus.io.data[HDMA2] = 0xFF;
      bus.io.data[HDMA3] = 0xFF;
      bus.io.data[HDMA4] = 0xFF;
      bus.io.data[HDMA5] = 0xFF;
      bus.io.data[RP]    = 0xFF;
      bus.io.data[BCPS]  = 0xC0;
      bus.io.data[BCPD]  = 0xFF;
      bus.io.data[OCPS]  = 0xC1;
      bus.io.data[OCPD]  = 0xA4;
      bus.io.data[SVBK]  = 0xF8;
      bus.io.data[IE]    = 0x00;

      cpu.AF = 0x1180;
      cpu.BC = 0x0000;
      cpu.DE = 0xFF56;
      cpu.HL = 0x000D;
      break;
    }
  }
  cpu.PC    = 0x0100;
  cpu.SP    = 0xFFFE;
}

GB::GB() {
  Mapper::bus = &bus;
  bus.apu     = &apu;
  cpu.ppu     = &ppu;
  cpu.timer = &timer;
  // apu.timer = &timer;
  timer.apu = &apu;
  cpu.bus     = &bus;
  ppu.bus     = &bus;
  
}
GB::~GB() {
  if (bus.cart.info.title.empty()) { return; }
  if (!(cpu.mapper->id == 0x03 || cpu.mapper->id == 0x06 ||
        cpu.mapper->id == 0x09 || cpu.mapper->id == 0x0D ||
        cpu.mapper->id == 0x0F || cpu.mapper->id == 0x13 ||
        cpu.mapper->id == 0x1B || cpu.mapper->id == 0x1E ||
        cpu.mapper->id == 0x22 || cpu.mapper->id == 0xFF)) {
    return;
  }

  if (!std::filesystem::exists("saves")) {
    std::filesystem::create_directory("saves");
  }

  // TODO: hash some unique bytes in ROM, save on that instead of ROM name
  // Open save file, truncate all content
  std::ofstream save(fmt::format("saves/{}.sav", bus.cart.info.title), 
                     std::ios::binary | std::ios::trunc);

  // Save SRAM to .sav file
  std::ostream_iterator<u8> output_iterator(save);
  std::copy(std::begin(bus.cart.ext_ram.data), std::end(bus.cart.ext_ram.data),
            output_iterator);

  save.close();
}
void GB::load_cart(const File &rom) {
  // set cartridge memory
  this->bus.cart.memory    = rom.data;
  this->bus.cart.info.path = rom.path;

  std::fill(this->bus.cart.ext_ram.data.begin(),
            this->bus.cart.ext_ram.data.end(), 0);

  // set compatibility flag
  this->bus.io.data[KEY0] = this->bus.cart.memory[0x143];
  if (this->bus.io.data[KEY0] == (u8)SYSTEM_MODE::CGB) {
    // experimental
    this->bus.mode = SYSTEM_MODE::CGB;
  } else {
    this->bus.mode = SYSTEM_MODE::DMG;
  }
  // initialize io registers depending system mode
  init_hw_regs(this->bus.mode);

  bus.cart.set_cart_info();
  fmt::println("HI");
  bus.cart.print_cart_info();
  fmt::println("HI2");
  Mapper *mapper_ptr = get_mapper_by_id(bus.cart.info.mapper_id);
  cpu.mapper         = mapper_ptr;
fmt::println("HI3");
  // ppu.mapper = mapper_ptr;

  // Load Save Game
  load_save_game();

  cpu.status = Umibozu::SM83::STATUS::ACTIVE;
}
void GB::load_save_game() {
  std::string save_path = fmt::format("saves/{}.sav", bus.cart.info.title);

  if (std::filesystem::exists(save_path)) {
    std::ifstream save(save_path, std::ios::binary);

    File save_file = read_file(save_path);
    u64 index      = 0;
    for (auto &byte : save_file.data) {
      bus.cart.ext_ram.data[index++] = byte;
    }

    fmt::println("[GB] save loaded");
  }
}
