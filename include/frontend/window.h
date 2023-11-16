#pragma once
#include <SDL2/SDL.h>

#include "common.h"
#include "core/gb.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

struct State {
  bool demo_window_open    = true;
  bool running             = true;
  SDL_Texture* ppu_texture = nullptr;
  bool texture_window_open = true;
  ImGuiIO* io;
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  GB* gb = nullptr;

  void handle_events();
  void render_frame();
  void show_viewport();

  Frontend();
  Frontend(GB *gb);
};
