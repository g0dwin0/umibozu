#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <unordered_map>

#include "core/gb.h"
#include "imgui.h"
#include "joypad.h"


struct State {
  SDL_Texture* ppu_texture = nullptr;
  SDL_Texture* viewport    = nullptr;
  const u8* keyboardState = SDL_GetKeyboardState(nullptr);

  bool running              = true;
  bool texture_window_open  = true;
  bool cpu_info_open        = true;
  bool ppu_info_open        = true;
  bool controls_window_open = false;

  
  

  ImGuiIO* io = nullptr;
};
struct Settings {
  struct Keybinds {
    // bool waiting_for_rebind   = false;
    std::vector<std::pair<BUTTONS, bool>> waiting_for_rebinds;

    
    BUTTONS button_to_remap = NONE;

    std::unordered_map<BUTTONS, std::pair<SDL_Scancode, bool>> control_map = 
    {
      {BUTTONS::A, {SDL_SCANCODE_Z, false}},
      {BUTTONS::B, {SDL_SCANCODE_X, false}},
      {BUTTONS::SELECT, {SDL_SCANCODE_BACKSPACE, false}},
      {BUTTONS::START, {SDL_SCANCODE_RETURN, false}},
      {BUTTONS::RIGHT,{ SDL_SCANCODE_RIGHT, false}},
      {BUTTONS::LEFT, {SDL_SCANCODE_LEFT, false}},
      {BUTTONS::UP, {SDL_SCANCODE_UP, false}},
      {BUTTONS::DOWN, {SDL_SCANCODE_DOWN, false}},
    };


  } keybinds;

};

struct Frontend {
  SDL_Window* window;
  SDL_Renderer* renderer;
  State state;
  Settings settings;
  GB* gb = nullptr;

  char const* patterns[2] = {"*.gb", "*.gbc"};

  void handle_events();
  void render_frame();

  void show_menubar();
  void show_viewport();
  void show_cpu_info();
  void show_ppu_info();
  void show_io_info();
  void show_tile_maps();
  void show_controls_menu(bool* p_open);
  void shutdown();



  explicit Frontend(GB*);
};
