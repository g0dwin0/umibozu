#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include <cassert>

#include "bus.h"
#include "common.h"
// #include "cpu.h"
#define VRAM_ADDRESS_OFFSET 0x8000
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
enum RENDERING_MODE : u8 { HBLANK = 0, VBLANK, OAM_SCAN, PIXEL_DRAW };
struct Sprite {
  u8 y_pos;
  u8 x_pos;
  u8 tile_no;
  u8 sprite_flags;
};
class PPU {
  RENDERING_MODE ppu_mode;
  SDL_Texture* frame_texture = nullptr;
  SDL_Renderer* renderer     = nullptr;
  std::array<Sprite, 0x20> sprite_buf;

  RENDERING_MODE get_mode() { return ppu_mode; }
  u8 get_ppu_mode();
  void set_ppu_mode(RENDERING_MODE mode);
  void add_sprite_to_buffer(u8 sprite_index);

 public:

  u8 sprite_count = 0;
  u8 oam_index    = 0;
  SDL_Texture* tile_map_0 = nullptr;
  SDL_Texture* tile_map_1 = nullptr;
  Bus* bus                = nullptr;
  struct LCDC lcdc;
  u16 dots;
  bool frame_queued = false;

  PPU();

  void tick();
  std::string get_mode_string();
  void set_renderer(SDL_Renderer* renderer);
  void set_frame_texture(SDL_Texture* texture);
  void render_tile_maps_to_texture();
  void get_dots();
  void render_frame();
};