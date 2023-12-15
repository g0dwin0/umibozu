#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

#include "CLI11.hpp"
#include "frontend/window.h"
#include "gb.h"
#include "io.hpp"
using namespace Umibozu;
int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "umibozu"};

  app.add_option("-f,--file", filename, "path to ROM");
  
  CLI11_PARSE(app, argc, argv);
  return 0;
}

int main(int argc, char** argv) {
  GB gb;
  Frontend fe(gb);

  std::string filename = "";
  handle_args(argc, argv, filename);
  fmt::println("{}", filename);
  
  gb.load_cart(read_file(filename));
  
  // OPTIMIZE: abstract this away

  fe.gb = &gb;
  gb.ppu.set_renderer(fe.renderer);
  gb.ppu.set_frame_texture(fe.state.ppu_texture);
  gb.ppu.set_sprite_overlay_texture(fe.state.sprite_overlay_texture);

  gb.ppu.bg_viewport     = fe.state.bg_viewport;
  gb.ppu.sprite_viewport = fe.state.sprite_viewport;

  while (fe.state.running) {
    gb.cpu.run_instruction();
    if (gb.ppu.frame_queued) {
      fe.handle_events();
      fe.render_frame();
      gb.ppu.frame_queued = false;
    }
  }

  fe.shutdown();

  return 0;
}
