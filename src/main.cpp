#include "CLI/CLI11.hpp"
#include "frontend/window.hpp"
#include "gb.hpp"
#include "io.hpp"
int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"", "umibozu"};
  app.add_option("-f,--file", filename, "path to ROM")->required();

  CLI11_PARSE(app, argc, argv);
  return 0;
}

using namespace Umibozu;

// #pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char** argv) {
  std::string filename = {};
  handle_args(argc, argv, filename);

  auto f = read_file(filename);

  GB gb = {};
  Frontend fe(&gb);

  gb.load_cart(f);
  gb.apu.stream = fe.stream;
  // std::thread system = std::thread(&GB::system_loop, &gb);

  while (fe.state.running) {
    gb.cpu.run_instruction();
    fe.handle_events();
    if (gb.ppu.frame_queued) {
      fe.render_frame();
      gb.ppu.frame_queued = false;
    }
  }

  // gb.active = false;
  // system.join();

  fe.shutdown();
}
