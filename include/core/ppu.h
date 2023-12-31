#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

#include <array>

#include "bus.h"
#include "common.h"
#include "mapper.h"
#define WHITE 0x6BFC
#define LIGHTGREY 0x3B11
#define DARKGREY 0x29A6
#define BLACK 0x0000

#define VRAM_ADDRESS_OFFSET 0x8000

struct LCDC_R {
  // REFACTOR: use unions
  u8 bg_and_window_enable_priority : 1;
  u8 sprite_enable                 : 1;
  u8 sprite_size                   : 1;
  u8 bg_tile_map_select            : 1;
  u8 tiles_select_method           : 1;

  u8 window_disp_enable            : 1;
  u8 window_tile_map_select        : 1;
  u8 lcd_ppu_enable                : 1;

  LCDC_R(u8 o) {
    bg_and_window_enable_priority = o & 0x1 ? 1 : 0;  
    sprite_enable                 = o & 0x2 ? 1 : 0;  
    sprite_size                   = o & 0x4 ? 1 : 0;  
    bg_tile_map_select            = o & 0x8 ? 1 : 0;  
    tiles_select_method           = o & 0x10 ? 1 : 0; 
    window_disp_enable            = o & 0x20 ? 1 : 0; 
    window_tile_map_select        = o & 0x40 ? 1 : 0; 
    lcd_ppu_enable                = o & 0x80 ? 1 : 0; 
  }
  LCDC_R& operator=(u8 v) {
    bg_and_window_enable_priority = v & 0x1 ? 1 : 0;
    sprite_enable                 = v & 0x2 ? 1 : 0;
    sprite_size                   = v & 0x4 ? 1 : 0;
    bg_tile_map_select            = v & 0x8 ? 1 : 0;
    tiles_select_method           = v & 0x10 ? 1 : 0;
    window_disp_enable            = v & 0x20 ? 1 : 0;
    window_tile_map_select        = v & 0x40 ? 1 : 0;
    lcd_ppu_enable                = v & 0x80 ? 1 : 0;
    return *this;
  };
};
struct Pixel {
  u8 color = 0;
};
struct Attribute_Data {
  u8 color_palette : 3 = 0;
  u8 bank          : 1 = 0;
  u8 x_flip        : 1 = 0;
  u8 y_flip        : 1 = 0;
  bool priority        = false;

  void set_from_byte(u8 idx) {
    color_palette = (idx & 0b00000111);
    bank          = (idx & 0b00001000) >> 3;
    x_flip        = (idx & 0b00100000) >> 5;
    y_flip        = (idx & 0b01000000) >> 6;
    priority      = ((idx & 0b10000000) >> 7) == 1;
  }
};
struct Tile {
  std::array<std::array<Pixel, 8>, 8> pixel_data;
  Attribute_Data attr_data;
};
enum class RENDERING_MODE : u8 { HBLANK = 0, VBLANK, OAM_SCAN, PIXEL_DRAW };
struct Sprite {
  u8 y_pos   = 0;
  u8 x_pos   = 0;
  u8 tile_no = 0;

  // TODO: add tile(s) to sprite

  u8 palette_number     : 1 = 0;
  u8 bank               : 1 = 0;
  u8 cgb_palette        : 3 = 0;

  u8 x_flip             : 1 = 0;
  u8 y_flip             : 1 = 0;
  u8 obj_to_bg_priority : 1 = 0;

  Sprite(u8 y_pos, u8 x_pos, u8 tile_no, u8 s_flags) {
    this->y_pos   = y_pos;
    this->x_pos   = x_pos;
    this->tile_no = tile_no;

    // CGB
    this->cgb_palette = s_flags & 0x7;          // 0 - 2
    this->bank        = s_flags & 0x8 ? 1 : 0;  // 3

    // DMG
    this->palette_number     = s_flags & 0x10 ? 1 : 0;  // 4
    this->x_flip             = s_flags & 0x20 ? 1 : 0;  // 5
    this->y_flip             = s_flags & 0x40 ? 1 : 0;  // 6
    this->obj_to_bg_priority = s_flags & 0x80 ? 1 : 0;  // 7
  }
};
struct Frame {
  std::array<u16, 256 * 256> data;
  std::array<u8, 256 * 256> color_id;
  std::array<bool, 256 * 256> bg_prio;

  void clear() {
    data.fill(0);
    color_id.fill(0);
    bg_prio.fill(false);
  }
  Frame() { data.fill(0x0); }
};

enum class FLIP_AXIS { X, Y };

typedef std::array<u16, 4> Palette;

struct SystemPalettes {
  std::array<Palette, 8> BGP = {};
  std::array<Palette, 8> OBP;

  SystemPalettes() { BGP[0] = {WHITE, LIGHTGREY, DARKGREY, BLACK}; }
  [[nodiscard]] Palette get_palette_by_id(u8 index) {
    assert(index < 8);
    return OBP[index];
  }
};
class PPU {
  RENDERING_MODE ppu_mode = RENDERING_MODE::VBLANK;

  SDL_Texture* frame_texture          = nullptr;
  SDL_Texture* sprite_overlay_texture = nullptr;
  SDL_Renderer* renderer              = nullptr;

  std::vector<Sprite> sprite_buf;
  u8 sprite_index = 0;
  void set_ppu_mode(RENDERING_MODE mode);
  void add_sprite_to_buffer(u8 sprite_index);
  u8 get_sprite_size() const;
  static void flip_sprite(Tile&, FLIP_AXIS);  // move to sprite class
  Tile active_tile;
  u8 w_y            = 0;
  u8 w_line_count   = 0;
  u8 w_x_pos_offset = 0;
  [[nodiscard]] u16 get_tile_bg_map_address_base() const;
  [[nodiscard]] u16 get_tile_window_map_address_base() const;

 public:
  Frame frame;
  Frame sprite_overlay;
  SystemPalettes sys_palettes;
  RENDERING_MODE get_mode() { return ppu_mode; }

  struct LCDC_R lcdc {
    0x91
  };
  u16 dots                     = 0;
  SDL_Texture* bg_viewport     = nullptr;
  SDL_Texture* sprite_viewport = nullptr;
  Bus* bus                     = nullptr;
  Mapper* mapper               = nullptr;

  bool frame_queued = false;
  bool frame_skip   = false;

  bool hdma_active     = false;
  u16 remaining_length = 0;
  u16 hdma_index       = 0;

  PPU();
  u8 x_pos_offset = 0;
  void tick(u16 inc);
  [[nodiscard]] std::string get_mode_string();
  void set_renderer(SDL_Renderer* renderer);
  void set_frame_texture(SDL_Texture* texture);
  void set_sprite_overlay_texture(SDL_Texture* texture);
  void process_hdma_chunk();
  void increment_scanline() const;
  [[nodiscard]] Tile get_tile_data(u16 address, bool sprite = false) const;
  [[nodiscard]] Tile get_tile_sprite_data(u16 index, bool sprite = false,
                                          u8 bank = 0) const;

  const std::array<u16, 4> shade_table = {WHITE, LIGHTGREY, DARKGREY, BLACK};
  bool window_enabled                  = false;
  static std::array<Pixel, 8> decode_pixel_row(u8 high_byte, u8 low_byte);
};