#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>

#include "SDL3/SDL_keyboard.h"
#include "core/gb.hpp"
#include "core/version.h"
#include "imgui.h"
#include "imgui_memory_edit.h"
#include "joypad.hpp"

struct State {
  SDL_Texture* ppu_texture = nullptr;
  SDL_Texture* viewport    = nullptr;

  const bool* keyboard_state = SDL_GetKeyboardState(NULL);

  bool running              = true;
  bool texture_window_open  = false;
  bool cpu_info_open        = false;
  bool ppu_info_open        = false;
  bool controls_window_open = false;
  bool memory_viewer_open   = false;
  bool io_info_open         = false;
  bool apu_info_open        = true;

  bool debug_windows_visible = false;

  size_t menu_bar_size = 0;

  ImGuiIO* io = nullptr;
};

struct Settings {
  struct Keybinds {
    std::unordered_map<BUTTONS, std::pair<SDL_Scancode, bool>> control_map = {
        {     BUTTONS::A,         {SDL_SCANCODE_Z, false}},
        {     BUTTONS::B,         {SDL_SCANCODE_X, false}},
        {BUTTONS::SELECT, {SDL_SCANCODE_BACKSPACE, false}},
        { BUTTONS::START,    {SDL_SCANCODE_RETURN, false}},
        { BUTTONS::RIGHT,     {SDL_SCANCODE_RIGHT, false}},
        {  BUTTONS::LEFT,      {SDL_SCANCODE_LEFT, false}},
        {    BUTTONS::UP,        {SDL_SCANCODE_UP, false}},
        {  BUTTONS::DOWN,      {SDL_SCANCODE_DOWN, false}},
    };

  } keybinds;
};

struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  Settings settings;
  GB* gb                  = nullptr;
  SDL_AudioStream* stream = nullptr;
  SDL_FRect dst;
  SDL_FRect src = {
      .x = 0,
      .y = 0,
      .w = 160,
      .h = 148,
  };

  Uint32 start_ticks, end_ticks;
  f32 fps;

  int screenWidth, screenHeight;

  char const* patterns[2] = {"*.gb", "*.gbc"};

  void handle_events();
  void render_frame();
  void show_menubar();
  void show_viewport();
  void show_cpu_info();
  void show_apu_info();
  void show_memory_viewer();
  void show_ppu_info();
  void show_io_info();
  void dump_framebuffer();
  // void show_tile_maps();
  void show_controls_menu(bool* p_open);
  void shutdown();
  void init_audio_device();

  SDL_AudioSpec spec;

  explicit Frontend(GB*);
};
