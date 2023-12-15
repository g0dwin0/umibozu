#include "core/gb.h"

#include <cassert>

#include "bus.h"
#include "common.h"

void GB::init_hw_regs() {
  bus.wram.data[P1]          = 0xCF;
  bus.wram.data[SB]          = 0x00;
  bus.wram.data[SC]          = 0x7E;
  bus.wram.data[UNUSED_FF03] = 0xFF;
  bus.wram.data[DIV]         = 0xAB;
  bus.wram.data[TIMA]        = 0x00;
  bus.wram.data[TMA]         = 0x00;
  bus.wram.data[TAC]         = 0xF8;
  bus.wram.data[IF]          = 0xE1;
  bus.wram.data[LCDC]        = 0x91;
  bus.wram.data[STAT]        = 0x85;
  bus.wram.data[SCY]         = 0x00;
  bus.wram.data[SCX]         = 0x00;
  bus.wram.data[LY]          = 0x00;
  bus.wram.data[LYC]         = 0x00;
  bus.wram.data[DMA]         = 0xFF;
  bus.wram.data[BGP]         = 0xFC;
  bus.wram.data[OBP0]        = 0x00;
  bus.wram.data[OBP1]        = 0x00;
  bus.wram.data[WY]          = 0x00;
  bus.wram.data[WX]          = 0x00;
  bus.wram.data[IE]          = 0x00;

  cpu.AF = 0x01B0;
  cpu.BC = 0x0013;
  cpu.DE = 0x00D8;
  cpu.HL = 0x014D;
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