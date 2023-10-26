#include <chrono>
#include <ratio>

#include "common.h"
#include "core/cart.h"
#include "core/cpu.h"
#include "io.hpp"
using namespace Umibozu;

struct GB {
  SharpSM83 cpu;
  Bus bus;

  GB() { cpu.bus = &bus; }

  void start() {
    u32 count = 100'000'000;
    while (count != 0) {
      cpu.run_instruction();
      count--;
    }
  }
};

int main() {
  GB gb;

  const std::string rom =
      "roms/gb-test-roms/cpu_instrs/individual/02-interrupts.gb";
  std::vector<u8> data = read_file(rom);
  gb.bus.cart.load_cart(data);

  gb.start();
  return 0;
}