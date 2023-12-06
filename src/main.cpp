#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstddef>

#include "common.h"
#include "frontend/window.h"
#include "gb.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  Frontend fe;

  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/dmg-acid2.gb"));


  // OPTIMIZE: abstract this away
  fe.gb = &gb;

  gb.ppu.set_renderer(fe.renderer);
  gb.ppu.set_frame_texture(fe.state.ppu_texture);
  gb.ppu.set_sprite_overlay_texture(fe.state.sprite_overlay_texture);
  
  gb.ppu.bg_viewport = fe.state.bg_viewport;
  gb.ppu.sprite_viewport = fe.state.sprite_viewport;
  
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