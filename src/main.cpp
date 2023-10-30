#include <chrono>
#include <ratio>

#include "common.h"
#include "core/gb.h"
#include "io.hpp"
using namespace Umibozu;


int main() {
  GB gb;

  const std::string rom =
      "/home/toast/Projects/umibozu/roms/gb-test-roms/cpu_instrs/individual/05-op rp.gb";
  std::vector<u8> data = read_file(rom);
  gb.bus.cart.load_cart(data);

  gb.start();
  return 0;
}