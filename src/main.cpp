#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "common.h"
#include "frontend/window.h"
#include "gb.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe;

  gb.load_cart(
      read_file("/home/toast/Projects/umibozu/roms/gb-test-roms/halt_bug.gb"));

  // OPTIMIZE: abstract this away
  fe.gb = &gb;

  gb.ppu.set_renderer(fe.renderer);
  gb.ppu.set_frame_texture(fe.state.ppu_texture);
  gb.ppu.tile_map_0 = fe.state.tile_map_texture_0;
  gb.ppu.tile_map_1 = fe.state.tile_map_texture_1;

  while (fe.state.running) {
    gb.cpu.run_instruction();
    if (gb.ppu.frame_queued) {
      fe.handle_events();
      fe.render_frame();
      gb.ppu.frame_queued = false;
    }
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
  return 0;
}