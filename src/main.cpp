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
  // gb.load_cart(read_file("/home/toast/Projects/umibozu/roms/dmg-acid2.gb"));

  // gb.start(0xFFFF);

  while (fe.state.running) {
    fe.handle_events();
    fe.render_frame();
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  // SDL_DestroyRenderer(renderer);
  // SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}