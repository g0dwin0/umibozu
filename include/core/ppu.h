#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include <queue>

#include "bus.h"
#include "common.h"
#define VRAM_ADDRESS_OFFSET 0x8000

struct LCDC_R {
  u8 bg_and_window_enable_priority : 1;  // 0
  u8 sprite_enable : 1;                  // 1
  u8 sprite_size : 1;                    // 2
  u8 bg_tile_map_select : 1;             // 3
  u8 tiles_select_method : 1;            // 4

  u8 window_disp_enable : 1;      // 5
  u8 window_tile_map_select : 1;  // 6
  u8 lcd_ppu_enable : 1;          // 7

  LCDC_R(u8 o) {
    // fmt::println("setting values with o: {:#04x}", o);
    bg_and_window_enable_priority = o & 0x1 ? 1 : 0;
    sprite_enable                 = o & 0x2 ? 1 : 0;
    sprite_size                   = o & 0x4 ? 1 : 0;
    bg_tile_map_select            = o & 0x8 ? 1 : 0;
    tiles_select_method           = o & 0x10 ? 1 : 0;
    window_disp_enable            = o & 0x20 ? 1 : 0;
    window_tile_map_select        = o & 0x40 ? 1 : 0;
    lcd_ppu_enable                = o & 0x80 ? 1 : 0;
  }
  void print_status() {
    return;
    fmt::println("\n===============LCDC=============");
    fmt::println("[LCDC.0] BG AND Window Enable / Priority = {:d}",
                 +bg_and_window_enable_priority);
    fmt::println("[LCDC.1] sprite enable = {:d}", +sprite_enable);
    fmt::println("[LCDC.2] sprite size = {:d}", +sprite_size);
    fmt::println("[LCDC.3] BG tile map range = {:d}", +bg_tile_map_select);
    fmt::println("[LCDC.4] addressing mode = {:d}", +tiles_select_method);
    fmt::println("[LCDC.5] window Enabled = {:d}", +window_disp_enable);
    fmt::println("[LCDC.6] window tile map range = {:d}",
                 +window_tile_map_select);
    fmt::println("[LCDC.7] LCD PPU enabled = {:d}", +lcd_ppu_enable);
    fmt::println("==================================\n");
  }
  LCDC_R& operator=(u8 o) {
    // fmt::println("o: {:#04x}", o);
    bg_and_window_enable_priority = o & 0x1 ? 1 : 0;
    sprite_enable                 = o & 0x2 ? 1 : 0;
    sprite_size                   = o & 0x4 ? 1 : 0;
    bg_tile_map_select            = o & 0x8 ? 1 : 0;
    tiles_select_method           = o & 0x10 ? 1 : 0;
    window_disp_enable            = o & 0x20 ? 1 : 0;
    window_tile_map_select        = o & 0x40 ? 1 : 0;
    lcd_ppu_enable                = o & 0x80 ? 1 : 0;
    return *this;
  };
};
struct Pixel {
  u8 color;
};
struct Tile {
  Pixel pixel_data[8][8];
};
enum PIXEL_TYPE { BG_PIXEL, WINDOW_PIXEL };
enum RENDERING_MODE : u8 { HBLANK = 0, VBLANK, OAM_SCAN, PIXEL_DRAW };
struct Sprite {
  u8 y_pos;
  u8 x_pos;
  u8 tile_no;
  u8 sprite_flags;
};
// struct Frame {
//   u8 data[160][144];
// };

class PPU {
  RENDERING_MODE ppu_mode    = OAM_SCAN;
  SDL_Texture* frame_texture = nullptr;
  SDL_Renderer* renderer     = nullptr;
  std::array<Sprite, 0x20> sprite_buf;
  bool had_window_pixels = false;

  u8 get_ppu_mode();
  void set_ppu_mode(RENDERING_MODE mode);
  void add_sprite_to_buffer(u8 sprite_index);

 public:
  // Frame frame;
  RENDERING_MODE get_mode() { return ppu_mode; }
  struct LCDC_R lcdc {
    0x91
  };
  u8 sprite_count         = 0;
  u8 sprite_index         = 0;
  u16 dots                = 0;
  SDL_Texture* tile_map_0 = nullptr;
  SDL_Texture* tile_map_1 = nullptr;
  Bus* bus                = nullptr;
  bool frame_queued       = false;
  u8 y_index              = 0;
  u8 x_index              = 0;

  PPU();
  void tick();
  std::string get_mode_string();
  void set_renderer(SDL_Renderer* renderer);
  void set_frame_texture(SDL_Texture* texture);
  void get_dots();
  void render_frame();
  void increment_scanline();
  Tile get_tile_data(u8 index);
  u16 get_tile_map_address_base();
  u8 x_pos_offset = 0;
  Tile active_tile;

  std::array<Pixel, 8> decode_pixel_row(u8 high_byte, u8 low_byte);
};