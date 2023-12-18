#include "core/gb.h"

#include <cassert>

#include "bus.h"
#include "common.h"

void GB::init_hw_regs() {
  bus.io.data[JOYPAD]      = 0xC7;
  bus.io.data[SB]          = 0x00;
  bus.io.data[SC]          = 0x7F;
  bus.io.data[UNUSED_FF03] = 0xFF;
  bus.io.data[DIV]         = 0x00;
  bus.io.data[TIMA]        = 0x00;
  bus.io.data[TMA]         = 0x00;
  bus.io.data[TAC]         = 0xF8;
  bus.io.data[IF]          = 0xE1;
  bus.io.data[LCDC]        = 0x91;
  bus.io.data[STAT]        = 0x00;
  bus.io.data[SCY]         = 0x00;
  bus.io.data[SCX]         = 0x00;
  bus.io.data[LY]          = 0x00;
  bus.io.data[LYC]         = 0x00;
  bus.io.data[DMA]         = 0x00;
  bus.io.data[BGP]         = 0xFC;
  bus.io.data[OBP0]        = 0x00;
  bus.io.data[OBP1]        = 0x00;
  bus.io.data[WY]          = 0x00;
  bus.io.data[WX]          = 0x00;
  bus.io.data[KEY0]        = 0xC0;
  bus.io.data[KEY1]        = 0xFF;
  bus.io.data[VBK]         = 0xFF;
  bus.io.data[HDMA1]       = 0xFF;
  bus.io.data[HDMA2]       = 0xFF;
  bus.io.data[HDMA3]       = 0xFF;
  bus.io.data[HDMA4]       = 0xFF;
  bus.io.data[HDMA5]       = 0xFF;
  bus.io.data[RP]          = 0xFF;
  bus.io.data[BCPS]        = 0x00;
  bus.io.data[BCPD]        = 0x00;
  bus.io.data[OCPS]        = 0x00;
  bus.io.data[OCPD]        = 0x00;
  bus.io.data[SVBK]        = 0xFF;
  bus.io.data[IE]          = 0x00;

  cpu.AF = 0x1180;
  cpu.BC = 0x0000;
  cpu.DE = 0xFF56;
  cpu.HL = 0x000D;
}

GB::GB() {
  Mapper::bus = &bus;
  cpu.ppu     = &ppu;
  cpu.bus     = &bus;
  ppu.bus     = &bus;
  init_hw_regs();
}
void GB::load_cart(const File& rom) {
  this->bus.cart.memory = rom.data;
  // TODO: verify cart integrity
  bus.cart.set_cart_info();
  bus.cart.print_cart_info();
  cpu.mapper = get_mapper_by_id(bus.cart.info.mapper_id);
}