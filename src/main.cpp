#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"

using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(&gb);

  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/ax6/rtc3test-1.gb"));
  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/ax6/rtc3test-2.gb"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/ax6/rtc3test-3.gb"));
  
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/MBC3_Test.gbc"));
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/mooneye/emulator-only/mbc1/bits_bank1.gb"));
  

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
