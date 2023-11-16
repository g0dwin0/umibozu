#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "common.h"
#include "frontend/window.h"
#include "gb.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  GB gb;
  gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/gb-test-roms/interrupt_time/interrupt_time.gb"));
  gb.start();
  // OPTIMIZE: abstract this away
  // Frontend fe;
  // fe.gb = &gb;
  
  // gb.bus.ppu.set_renderer(fe.renderer);
  // gb.bus.ppu.set_frame_texture(fe.state.ppu_texture);

  // while (fe.state.running) {
    // fe.handle_events();
    // gb.bus.ppu.render_frame();
    // fe.render_frame();
  // }

  // ImGui_ImplSDLRenderer2_Shutdown();
  // ImGui_ImplSDL2_Shutdown();
  // ImGui::DestroyContext();

  // SDL_Quit();
  return 0;
}