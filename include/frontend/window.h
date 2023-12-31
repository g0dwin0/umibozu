#pragma once
#include <SDL2/SDL.h>

#include "core/gb.h"
#include "imgui.h"
struct State {
  SDL_Texture* ppu_texture = nullptr;
  SDL_Texture* viewport    = nullptr;

  bool running             = true;
  bool texture_window_open = true;
  bool cpu_info_open       = true;
  bool ppu_info_open       = true;

  ImGuiIO* io = nullptr;
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  GB* gb = nullptr;

  char const* patterns[1] = {"*.gb"};

  void handle_events();
  void render_frame();

  void show_menubar();
  void show_viewport();
  void show_cpu_info();
  void show_ppu_info();
  void show_io_info();
  void show_tile_maps();

  void shutdown();

  explicit Frontend(GB&);
};
