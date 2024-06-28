#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"

using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(&gb);

  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/cpu_instrs/01-special.gb"));

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

  return 0;
}
