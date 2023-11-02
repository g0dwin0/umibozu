#include <chrono>

#include "common.h"
#include "core/gb.h"
#include "io.hpp"
using namespace Umibozu;


int main() {
  GB gb;

  const std::string rom =
      "/home/toast/Projects/umibozu/roms/gb-test-roms/cpu_instrs/individual/01-special.gb";
  std::vector<u8> data = read_file(rom);
  gb.bus.cart.load_cart(data);

  gb.start();
  return 0;
}