#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "common.h"
struct LCDC {
  u8 bg_window_enable_priority : 1;
  u8 obj_enable : 1;
  u8 obj_size : 1;
  u8 bg_tile_map : 1;
  u8 bg_window_tiles : 1;
  u8 window_enable : 1;
  u8 window_tile_map : 1;
  u8 lcd_ppu_enable : 1;

  void print_status() {
    return;
    fmt::println("\n===============LCDC=============");
    fmt::println("BG Window Enable / Priority = {:d}",
                 +bg_window_enable_priority);
    fmt::println("OBJ Enable = {:d}", +obj_enable);
    fmt::println("OBJ Size = {:d}", +obj_size);
    fmt::println("BG Tile Map = {:d}", +bg_tile_map);
    fmt::println("BG Window Tiles = {:d}", +bg_window_tiles);
    fmt::println("Window Enabled = {:d}", +window_enable);
    fmt::println("BG Window Tiles = {:d}", +bg_window_tiles);
    fmt::println("LCD PPU enabled = {:d}", +lcd_ppu_enable);
    fmt::println("==================================\n");
  }
  LCDC& operator=(u8 o) {
    bg_window_enable_priority = o & 0x1 ? 1 : 0;
    obj_enable                = o & 0x2 ? 1 : 0;
    obj_size                  = o & 0x4 ? 1 : 0;
    bg_tile_map               = o & 0x8 ? 1 : 0;
    bg_window_tiles           = o & 0x10 ? 1 : 0;
    window_enable             = o & 0x20 ? 1 : 0;
    window_tile_map           = o & 0x40 ? 1 : 0;
    lcd_ppu_enable            = o & 0x80 ? 1 : 0;
    return *this;
  };
};
class PPU {
  enum class RENDERING_MODE {
    HORIZONTAL_BLANK = 0,
    WAIT_NEXT_FRAME,
    OAM_SCAN,
    PIXEL_DRAW
  };
  SDL_Texture* frame_texture = nullptr;
  SDL_Renderer* renderer     = nullptr;

  bool frame_ready = false;

 public:
  PPU();
  RAM* vram = nullptr;
  struct LCDC lcdc;
  void set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
  void set_frame_texture(SDL_Texture* texture) {
    this->frame_texture = texture;
  }

  void render_frame();
};