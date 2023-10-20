#include "common.h"
#include "core/cart.h"
#include "core/cpu.h"
#include "io.hpp"

using namespace Umibozu;

// As the Game Boy’s 16-bit address bus offers only limited space for ROM and
// RAM addressing, many games are using Memory Bank Controllers (MBCs) to expand
// the available address space by bank switching. These MBC chips are located in
// the game cartridge (that is, not in the Game Boy itself).

struct GB {
  SharpSM83 cpu;
  Cartridge cart;
  Bus bus;

  GB() {
    cpu.bus = &bus;
    bus.cart = &cart;
  }

  void start() {
    while (true) {
      cpu.run_instruction();
    }
  }
};

int main() {
  GB gb;

  const std::string rom = "roms/gb-test-roms/cpu_instrs/cpu_instrs.gb";
  std::vector<u8> data  = read_file(rom);
  gb.bus.cart->load_cart(data);

  gb.start();
  return 0;
}