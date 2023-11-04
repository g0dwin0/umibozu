#include <SDL2/SDL.h>
#include "common.h"
#include "core/gb.h"
#include "frontend/window.h"
#include "imgui.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/dmg-acid2.gb"));

  gb.start(0xFFFF);

  return 0;
}