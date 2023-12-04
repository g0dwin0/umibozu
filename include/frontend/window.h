#pragma once
#include <SDL2/SDL.h>

#include "common.h"
#include "core/gb.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "tinyfiledialogs.h"
struct State {
  SDL_Texture* ppu_texture            = nullptr;
  SDL_Texture* sprite_overlay_texture = nullptr;

  SDL_Texture* bg_viewport     = nullptr;
  SDL_Texture* sprite_viewport = nullptr;
  SDL_Surface* surface         = nullptr;
  bool demo_window_open        = true;
  bool running                 = true;
  bool texture_window_open     = true;
  bool cpu_info_open           = true;
  bool ppu_info_open           = true;
  bool debug_log_window_open   = true;

  ImGuiIO* io;
};
struct Config {
  char const* patterns[2] = {"*.gb", "*.gbc"};
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  GB* gb = nullptr;
  Config config;

  void handle_events();
  void render_frame();

  void show_menubar();
  void show_viewport();
  void show_cpu_info();
  void show_ppu_info();
  void show_tile_maps();

  Frontend();
  Frontend(GB* gb);
};
