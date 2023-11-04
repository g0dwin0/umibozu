#include <SDL2/SDL.h>
#include "common.h"
#include "core/gb.h"
#include "frontend/window.h"
#include "imgui.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/gb-test-roms/instr_timing/instr_timing.gb"));

  gb.start();

  return 0;
}