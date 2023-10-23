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
    u64 count = 1530000;
    while (count != 0) {
      cpu.run_instruction();
      count--;
    }
  }
};

int main() {
  GB gb;


  const std::string rom = "/home/toast/Projects/umibozu/roms/gb-test-roms/cpu_instrs/individual/01-special.gb";
  std::vector<u8> data  = read_file(rom);
  gb.bus.cart.load_cart(data);

  gb.start();
  return 0;
}