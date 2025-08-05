#include "cli11/CLI11.hpp"
#include "cpu.hpp"
#include "frontend/window.hpp"
#include "gb.hpp"

int handle_args(int& argc, char** argv, std::string& filename) {
  CLI::App app{"a rudimentary gameboy emulator", "umibozu"};
  app.add_option("-f,--file", filename, "path to ROM")->required();

  CLI11_PARSE(app, argc, argv);
  return 0;
}




int main(int argc, char** argv) {
  GB gb = {};
  Frontend fe(&gb);

  std::string filename = {};
  handle_args(argc, argv, filename); 

  File rom = read_file(filename);
  
  gb.load_cart(rom);
  

  while (fe.state.running) {
    // if (gb.cpu.status == SM83::STATUS::ACTIVE) {
    // }
    gb.cpu.step();
    fe.handle_events();
    fe.render_frame();
    // if (gb.ppu.frame_queued || gb.cpu.status == SM83::STATUS::PAUSED) {
    //   gb.ppu.frame_queued = false;
    // }
  }

  fe.shutdown();
}
