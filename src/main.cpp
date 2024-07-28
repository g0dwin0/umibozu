#include "CLI11.hpp"
#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"

using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(&gb);
  // http://slack.net/~ant/bl-synth/2.square.html

  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/01-registers.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/02-len_ctr.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/03-trigger.gb"));
  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/04-sweep.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/05-sweep_details.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/06-overflow_on_trigger.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/07-len_sweep_period_sync.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/08-len_ctr_during_power.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/09-wave_read_while_on.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/10-wave_trigger_while_on.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/blarg/dmg_sound/11-regs_after_power.gb"));

  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/acid/dmg-acid2.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/acid/which.gb"));

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
