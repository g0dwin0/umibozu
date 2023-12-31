// #include "CLI11.hpp"
#include "cpu.h"
#include "frontend/window.h"
#include "gb.h"
// #include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe(gb);

  fe.gb = &gb;
  gb.ppu.set_renderer(fe.renderer);
  gb.ppu.set_frame_texture(fe.state.ppu_texture);

  while (fe.state.running) {
    if (gb.cpu.status == Umibozu::SM83::Status::ACTIVE) {
      gb.cpu.run_instruction();
    }
    if (gb.ppu.frame_queued || gb.cpu.status == Umibozu::SM83::Status::PAUSED) {
      fe.handle_events();
      fe.render_frame();
      gb.ppu.frame_queued = false;
    }
  }

  fe.shutdown();

  return 0;
}
