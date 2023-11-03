#include <SDL2/SDL.h>
#include "common.h"
#include "core/gb.h"
#include "frontend/window.h"
#include "imgui.h"
#include "io.hpp"
using namespace Umibozu;

int main() {
  Frontend frontend;


  while (frontend.state.running != false) {
    frontend.handle_events();
    frontend.render_frame();
  }

  SDL_Quit();
  return 0;
}