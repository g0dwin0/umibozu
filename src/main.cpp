// #include "CLI11.hpp"
#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"

using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(&gb);

  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/08-len_ctr_during_power.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/09-wave_read_while_on.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/10-wave_trigger_while_on.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/dmg_sound.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/dmg_sound.gb"));
  

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
