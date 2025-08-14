#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>

#include <queue>
#include <unordered_map>

#include "core/gb.hpp"
#include "imgui.h"
#include "imgui_memory_edit.h"
#include "joypad.hpp"

struct State {
  SDL_Texture* ppu_texture    = nullptr;
  SDL_Texture* viewport       = nullptr;

  const u8* keyboardState = SDL_GetKeyboardState(nullptr);

  bool running              = true;
  bool texture_window_open  = false;
  bool cpu_info_open        = false;
  bool ppu_info_open        = false;
  bool controls_window_open = false;
  bool memory_viewer_open   = false;
  bool io_info_open         = false;

  bool debug_windows_visible        = false;
  SDL_AudioDeviceID audio_device_id = 0;

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
  GB* gb = nullptr;

  SDL_Rect dst;
  SDL_Rect src = {
      .x = 0,
      .y = 0,
      .w = 160,
      .h = 148,
  };

  int screenWidth, screenHeight;

  char const* patterns[2] = {"*.gb", "*.gbc"};

  void handle_events();
  void render_frame();
  void show_menubar();
  void show_viewport();
  void show_cpu_info();
  void show_memory_viewer();
  void show_ppu_info();
  void show_io_info();
  // void show_tile_maps();
  void show_controls_menu(bool* p_open);
  void shutdown();
  void init_audio_device();

  explicit Frontend(GB*);
};
