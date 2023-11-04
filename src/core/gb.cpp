#include "core/gb.h"
#include "bus.h"

#include <cassert>
#include <cstddef>

// using namespace Umibozu;

void GB::init_hw_regs() {
  bus.ram.data[P1]   = 0xCF;
  bus.ram.data[SB]   = 0x00;
  bus.ram.data[SC]   = 0x7E;
  bus.ram.data[DIV]  = 0xAB;
  bus.ram.data[TIMA] = 0x00;
  bus.ram.data[TMA]  = 0x00;
  bus.ram.data[TAC]  = 0xF8;
  bus.ram.data[IF]   = 0xE1;
  bus.ram.data[LCDC] = 0x91;
  bus.ram.data[STAT] = 0x81;
  bus.ram.data[SCY]  = 0x00;
  bus.ram.data[SCX]  = 0x00;
  bus.ram.data[LY]   = 0x91;
  bus.ram.data[LYC]  = 0x00;
  bus.ram.data[DMA]  = 0xFF;
  bus.ram.data[BGP]  = 0xFC;
  bus.ram.data[OBP0] = 0x00;
  bus.ram.data[OBP1] = 0x00;
  bus.ram.data[WY]   = 0x00;
  bus.ram.data[WX]   = 0x00;
  bus.ram.data[IE]   = 0x00;

  cpu.AF = 0x01B0;
  cpu.BC = 0x0013;
  cpu.DE = 0x00D8;
  cpu.HL = 0x014D;
}

GB::GB() {
  Mapper::bus = &bus;
  cpu.bus     = &bus;
  init_hw_regs();
}
void GB::load_cart(std::vector<u8> cart_data) {
  this->bus.cart.memory = cart_data;
  //TODO: verify cart integrity
  bus.cart.set_cart_info();
  cpu.mapper = get_mapper_by_id(bus.cart.info.mapper_id);
}
void GB::start(u64 count) {
  while (count != 0) {
    cpu.run_instruction();
    count--;
  }
}