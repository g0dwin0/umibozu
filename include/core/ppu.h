#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

#include <queue>
#include <vector>

#include "bus.h"
#include "common.h"
#define WHITE     0xFFFFFF00
#define LIGHTGREY 0xAAAAAA00
#define DARKGREY  0x55555500
#define BLACK     0x00000000

#define MAX_ALPHA 0xFF
#define MIN_ALPHA 0x00
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
    bg_and_window_enable_priority = o & 0x1 ? 1 : 0;   // 0
    sprite_enable                 = o & 0x2 ? 1 : 0;   // 1
    sprite_size                   = o & 0x4 ? 1 : 0;   // 2
    bg_tile_map_select            = o & 0x8 ? 1 : 0;   // 3
    tiles_select_method           = o & 0x10 ? 1 : 0;  // 4
    window_disp_enable            = o & 0x20 ? 1 : 0;  // 5
    window_tile_map_select        = o & 0x40 ? 1 : 0;  // 6
    lcd_ppu_enable                = o & 0x80 ? 1 : 0;  // 7
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
  u8 y_pos   = 0;
  u8 x_pos   = 0;
  u8 tile_no = 0;

  u8 palette_number : 1     = 0;
  u8 x_flip : 1             = 0;
  u8 y_flip : 1             = 0;
  u8 obj_to_bg_priority : 1 = 0;

  Sprite(){

  };
  Sprite(u8 y_pos, u8 x_pos, u8 tile_no, u8 s_flags) {
    this->y_pos   = y_pos;
    this->x_pos   = x_pos;
    this->tile_no = tile_no;

    this->palette_number     = s_flags & 0x10 ? 1 : 0;  // 4
    this->x_flip             = s_flags & 0x20 ? 1 : 0;  // 5
    this->y_flip             = s_flags & 0x40 ? 1 : 0;  // 6
    this->obj_to_bg_priority = s_flags & 0x80 ? 1 : 0;  // 7

    // fmt::println("\n====================");
    // fmt::println("Y POS: {:d}", +this->y_pos);
    // fmt::println("X POS: {:d}", +this->x_pos);
    // fmt::println("Tile No.: {:d}", +this->tile_no);

    // fmt::println("Palette Number: {:d}", +this->palette_number);
    // fmt::println("X Flip: {:d}", +this->x_flip);
    // fmt::println("Y Flip: {:d}", +this->y_flip);
    // fmt::println("OBJ to BG Priority: {:d}", +this->obj_to_bg_priority);
    // fmt::println("====================\n");
  }
};
struct Frame {
  std::array<u32, 256 * 256> data;
  std::array<u8, 256 * 256> color_id;
  
};

enum class FLIP_AXIS { X, Y, X_Y };
enum class LAYER { BG, WINDOW, SPRITE };

class PPU {
  RENDERING_MODE ppu_mode             = OAM_SCAN;
  SDL_Texture* frame_texture          = nullptr;
  SDL_Texture* sprite_overlay_texture = nullptr;

  SDL_Renderer* renderer = nullptr;
  std::array<Sprite, 10> sprite_buf;
  bool had_window_pixels = false;
  u8 sprite_count        = 0;
  u8 sprite_index        = 0;

  u8 get_ppu_mode();
  void set_ppu_mode(RENDERING_MODE mode);
  void add_sprite_to_buffer(u8 sprite_index);

 public:
  Frame frame;
  Frame sprite_overlay;
  RENDERING_MODE get_mode() { return ppu_mode; }
  struct LCDC_R lcdc {
    0x91
  };
  u16 dots                     = 0;
  SDL_Texture* bg_viewport     = nullptr;
  SDL_Texture* sprite_viewport = nullptr;
  Bus* bus                     = nullptr;
  bool frame_queued            = false;
  u8 y_index                   = 0;
  u8 x_index                   = 0;

  PPU();
  void tick();
  std::string get_mode_string();
  void set_renderer(SDL_Renderer* renderer);
  void set_frame_texture(SDL_Texture* texture);
  void set_sprite_overlay_texture(SDL_Texture* texture);
  void get_dots();
  void render_frame();
  void increment_scanline();
  Tile get_tile_data(u8 index, bool sprite = false);
  u16 get_tile_bg_map_address_base();
  u16 get_tile_window_map_address_base();
  void get_color_from_palette(const Sprite&);
  Tile flip_sprite(Tile, FLIP_AXIS);
  LAYER currentLayer;
  const std::array<u32, 4> shade_table = {WHITE, LIGHTGREY, DARKGREY,BLACK};


  std::array<u32, 4> BGP   = {WHITE + MAX_ALPHA, LIGHTGREY + MAX_ALPHA,
                              DARKGREY + MAX_ALPHA, BLACK + MAX_ALPHA};
  std::array<u32, 4> OBP_0 = {WHITE + MIN_ALPHA, LIGHTGREY + MAX_ALPHA,
                              DARKGREY + MAX_ALPHA, BLACK + MAX_ALPHA};
  std::array<u32, 4> OBP_1 = {WHITE + MIN_ALPHA, LIGHTGREY + MAX_ALPHA,
                              DARKGREY + MAX_ALPHA, BLACK + MAX_ALPHA};
  bool window_enabled = false;
  u8 x_pos_offset     = 0;
  u8 w_y              = 0;
  u8 w_line_count     = 0;
  u8 w_x_pos_offset   = 0;
  Tile active_tile;
  bool done = false;
  std::array<Pixel, 8> decode_pixel_row(u8 high_byte, u8 low_byte);
};