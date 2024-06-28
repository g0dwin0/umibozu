#include "core/gb.h"

#include <filesystem>

#include "bus.h"
#include "cpu.h"

void GB::init_hw_regs(SYSTEM_MODE mode) {
  switch (mode) {
  case SYSTEM_MODE::DMG: {
    bus.io.data[JOYPAD] = 0xCF;
    bus.io.data[SB] = 0x00;
    bus.io.data[SC] = 0x7E;
    bus.io.data[UNUSED_FF03] = 0xFF;
    bus.io.data[DIV] = 0x1E;
    bus.io.data[TIMA] = 0x00;
    bus.io.data[TMA] = 0x00;
    bus.io.data[TAC] = 0xF8;
    bus.io.data[IF] = 0xE9;
    bus.io.data[LCDC] = 0x91;
    bus.io.data[STAT] = 0x85;
    bus.io.data[SCY] = 0x00;
    bus.io.data[SCX] = 0x00;
    bus.io.data[LY] = 0x90;
    bus.io.data[LYC] = 0x00;
    bus.io.data[DMA] = 0xFF;
    bus.io.data[BGP] = 0xFC;
    bus.io.data[OBP0] = 0xFF;
    bus.io.data[OBP1] = 0xFF;
    bus.io.data[WY] = 0x00;
    bus.io.data[WX] = 0x00;
    // bus.io.data[KEY0]        = 0x80;
    bus.io.data[KEY1] = 0x7E;
    bus.io.data[VBK] = 0xFE;
    bus.io.data[HDMA1] = 0xFF;
    bus.io.data[HDMA2] = 0xFF;
    bus.io.data[HDMA3] = 0xFF;
    bus.io.data[HDMA4] = 0xFF;
    bus.io.data[HDMA5] = 0xFF;
    bus.io.data[RP] = 0xFF;
    bus.io.data[BCPS] = 0xC0;
    bus.io.data[BCPD] = 0xFF;
    bus.io.data[OCPS] = 0xC1;
    bus.io.data[OCPD] = 0x0D;
    bus.io.data[SVBK] = 0xFF;
    bus.io.data[IE] = 0x00;

    cpu.AF = 0x01B0;
    cpu.BC = 0x0013;
    cpu.DE = 0x00D8;
    cpu.HL = 0x014D;
    break;
  }
  case SYSTEM_MODE::CGB_ONLY: {
    bus.io.data[JOYPAD] = 0xCF;
    bus.io.data[SB] = 0x00;
    bus.io.data[SC] = 0x7F;
    bus.io.data[UNUSED_FF03] = 0xFF;
    bus.io.data[DIV] = 0x1E;
    bus.io.data[TIMA] = 0x00;
    bus.io.data[TMA] = 0x00;
    bus.io.data[TAC] = 0xF8;
    bus.io.data[IF] = 0xE1;

    bus.io.data[NR10] = 0x80;
    bus.io.data[NR11] = 0xBF;
    bus.io.data[NR12] = 0xF3;
    bus.io.data[NR13] = 0xFF;
    bus.io.data[NR14] = 0xBF;
    bus.io.data[NR21] = 0x3F;
    bus.io.data[NR22] = 0x00;
    bus.io.data[NR23] = 0xFF;
    bus.io.data[NR24] = 0xBF;
    bus.io.data[NR30] = 0x7F;
    bus.io.data[NR31] = 0xFF;
    bus.io.data[NR32] = 0x9F;
    bus.io.data[NR33] = 0xFF;
    bus.io.data[NR34] = 0xBF;
    bus.io.data[NR41] = 0xFF;
    bus.io.data[NR42] = 0x00;
    bus.io.data[NR43] = 0x00;
    bus.io.data[NR44] = 0xBF;
    bus.io.data[NR50] = 0x77;
    bus.io.data[NR51] = 0xF3;
    bus.io.data[NR52] = 0xF1;

    bus.io.data[LCDC] = 0x91;
    bus.io.data[STAT] = 0x81;
    bus.io.data[SCY] = 0x00;
    bus.io.data[SCX] = 0x00;
    bus.io.data[LY] = 0x90;
    bus.io.data[LYC] = 0x00;
    bus.io.data[DMA] = 0x00;
    bus.io.data[BGP] = 0xFC;
    bus.io.data[OBP0] = 0x00;
    bus.io.data[OBP1] = 0x00;
    bus.io.data[WY] = 0x00;
    bus.io.data[WX] = 0x00;
    // bus.io.data[KEY0]        = 0x80;
    bus.io.data[KEY1] = 0x7E;
    bus.io.data[VBK] = 0xFE;
    bus.io.data[HDMA1] = 0xFF;
    bus.io.data[HDMA2] = 0xFF;
    bus.io.data[HDMA3] = 0xFF;
    bus.io.data[HDMA4] = 0xFF;
    bus.io.data[HDMA5] = 0xFF;
    bus.io.data[RP] = 0xFF;
    bus.io.data[BCPS] = 0xC0;
    bus.io.data[BCPD] = 0xFF;
    bus.io.data[OCPS] = 0xC1;
    bus.io.data[OCPD] = 0xA4;
    bus.io.data[SVBK] = 0xF8;
    bus.io.data[IE] = 0x00;

    cpu.AF = 0x1180;
    cpu.BC = 0x0000;
    cpu.DE = 0xFF56;
    cpu.HL = 0x000D;
    break;
  }
  }
  cpu.PC = 0x0100;
  cpu.SP = 0xFFFE;
  cpu.timer = {};
}

GB::GB() {
  Mapper::bus = &bus;
  cpu.ppu = &ppu;
  cpu.bus = &bus;
  ppu.bus = &bus;
}
GB::~GB() {
  if (bus.cart.info.title.empty()) {
    return;
  }

  if (!std::filesystem::exists("saves")) {
    std::filesystem::create_directory("saves");
  }

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
  this->bus.cart.memory = rom.data;
  this->bus.cart.info.path = rom.path;

  std::fill(this->bus.cart.ext_ram.data.begin(),
            this->bus.cart.ext_ram.data.end(), 0);

  // set compatibility flag
  this->bus.io.data[KEY0] = this->bus.cart.memory[0x143];
  if (this->bus.io.data[KEY0] == (u8)SYSTEM_MODE::CGB_ONLY) {
    // experimental
    this->bus.mode = SYSTEM_MODE::CGB_ONLY;
  } else {
    this->bus.mode = SYSTEM_MODE::DMG;
  }

  // initialize io registers depending system mode
  init_hw_regs(this->bus.mode);

  bus.cart.set_cart_info();
  bus.cart.print_cart_info();
  Mapper *mapper_ptr = get_mapper_by_id(bus.cart.info.mapper_id);
  cpu.mapper = mapper_ptr;

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
    u64 index = 0;
    for (auto &byte : save_file.data) {
      bus.cart.ext_ram.data[index++] = byte;
    }

    fmt::println("save loaded");
  }
}
