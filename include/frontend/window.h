#pragma once
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>
#include "common.h"

struct State {
  bool demo_window_open = true;
  bool running = true;
  ImGuiIO* io;
};
struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;

  void handle_events();
  void render_frame();
  
  Frontend();
};
