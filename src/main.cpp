#include "common.h"
#include "core/cart.h"
#include "core/cpu.h"
#include "io.hpp"

using namespace Umibozu;

struct GB {
  SharpSM83 cpu;
  Bus bus;

  GB() {
    cpu.bus = &bus;
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
  gb.bus.cart.load_cart(data);

  gb.start();
  return 0;
}