#include "core/gb.h"

using namespace Umibozu;

void GB::init_hw_regs() {
  bus.ram.ram[P1]   = 0xCF;
  bus.ram.ram[SB]   = 0x00;
  bus.ram.ram[SC]   = 0x7E;
  bus.ram.ram[DIV]  = 0xAB;
  bus.ram.ram[TIMA] = 0x00;
  bus.ram.ram[TMA]  = 0x00;
  bus.ram.ram[TAC]  = 0xF8;
  bus.ram.ram[IF]   = 0xE1;
  bus.ram.ram[LCDC] = 0x91;
  bus.ram.ram[STAT] = 0x81;
  bus.ram.ram[SCY]  = 0x00;
  bus.ram.ram[SCX]  = 0x00;
  bus.ram.ram[LY]   = 0x91;
  bus.ram.ram[LYC]  = 0x00;
  bus.ram.ram[DMA]  = 0xFF;
  bus.ram.ram[BGP]  = 0xFC;
  bus.ram.ram[OBP0] = 0x00;
  bus.ram.ram[OBP1] = 0x00;
  bus.ram.ram[WY]   = 0x00;
  bus.ram.ram[WX]   = 0x00;
  bus.ram.ram[IE]   = 0x00;
}

void GB::start() {
  init_hw_regs();
  u32 count = 287'999;
  while (count != 0) {
    cpu.run_instruction();
    count--;
  }
}