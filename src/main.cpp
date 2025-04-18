// #include "CLI11.hpp"
#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"

using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(&gb);

  gb.load_cart(read_file("/home/bass/Projects/umibozu/roms/acid/dmg-acid2.gb"));
  

  while (fe.state.running) {
    if (gb.cpu.status == Umibozu::SM83::STATUS::ACTIVE) {
      gb.cpu.run_instruction();
    }
    if (gb.ppu.frame_queued || gb.cpu.status == Umibozu::SM83::STATUS::PAUSED) {
      fe.handle_events();
      fe.render_frame();
      gb.ppu.frame_queued = false;
    }
  }

  fe.shutdown();
}
