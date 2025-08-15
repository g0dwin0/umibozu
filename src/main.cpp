#include <thread>

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

#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char** argv) {
  // std::string filename = {};
  // handle_args(argc, argv, filename);

  // auto f = read_file(filename);

  GB gb = {};
  Frontend fe(&gb);

  // gb.load_cart(f);

  std::thread system = std::thread(&GB::system_loop, &gb);

  while (fe.state.running) {
    fe.render_frame();
    fe.handle_events();
  }

  gb.active = false;
  system.join();

  fe.shutdown();
}
