#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

#include <array>

#include "stopwatch.hpp"

struct Bus;
#include "bus.hpp"
#include "common.hpp"
#include "double_buffer.hpp"

class Mapper;
#include "mapper.hpp"

static constexpr u16 WHITE     = 0x6BFC;
static constexpr u16 LIGHTGREY = 0x3B11;
static constexpr u16 DARKGREY  = 0x29A6;
static constexpr u16 BLACK     = 0x0000;

static constexpr u16 VRAM_ADDRESS_OFFSET = 0x8000;

struct LCDC_R {
  // this doesn't need to be a struct, this could just a be union,
  // besides this should be on the bus as it's a memory mapped io register
  union {
    u8 value;
    struct {
      u8 bg_and_window_enable_priority : 1;
      bool sprite_enable               : 1;
      u8 sprite_size                   : 1;
      u8 bg_tile_map_select            : 1;
      u8 tiles_select_method           : 1;

      bool window_disp_enable          : 1;
      u8 window_tile_map_select        : 1;
      bool lcd_ppu_enable              : 1;
    };
  };
};

using Pixel = u8;

union Attribute_Data {
  u8 value = 0;
  struct {
    u8 color_palette : 3;
    u8 bank          : 1;
    u8 x_flip        : 1;
    u8 y_flip        : 1;
    bool priority    : 1;
  };
};
struct Tile {
  std::array<std::array<Pixel, 8>, 8> pixel_data;
  Attribute_Data cgb_attr_data;
};

enum class RENDERING_MODE { HBLANK = 0, VBLANK, OAM_SCAN, PIXEL_DRAW };

struct Sprite {
  u8 y_pos   = 0;
  u8 x_pos   = 0;
  u8 tile_no = 0;
  union {
    u8 value;
    struct {
      u8 cgb_palette        : 3 = 0;
      u8 bank               : 1 = 0;
      u8 palette_number     : 1 = 0;
      u8 x_flip             : 1 = 0;
      u8 y_flip             : 1 = 0;
      u8 obj_to_bg_priority : 1 = 0;
    };
  };

  Sprite(u8 y_pos, u8 x_pos, u8 tile_no, u8 s_flags) {
    // this->sprite_index = sprite_index;
    this->y_pos   = y_pos;
    this->x_pos   = x_pos;
    this->tile_no = tile_no;
    this->value   = s_flags;
  }
};
struct Frame {
  Frame();

  std::array<u16, 256 * 256> data;      // refactor: what is data? give more descriptive name
  std::array<u8, 256 * 256> color_id;   // refactor: is this a cgb thing?  more descriptive
  std::array<bool, 256 * 256> bg_prio;  // refactor: ??? what does this mean?
                                        // does the bg have prio, or the obj?

  void clear() {
    data.fill(0);
    color_id.fill(0);
    bg_prio.fill(false);
  }
};

enum class FLIP_AXIS { X, Y };

typedef std::array<u16, 4> Palette;

struct SystemPalettes {
  std::array<Palette, 8> BGP;
  std::array<Palette, 8> OBP;

  SystemPalettes();

  [[nodiscard]] Palette get_palette_by_id(u8 index);
};

struct PPU {
  RENDERING_MODE ppu_mode = RENDERING_MODE::VBLANK;

  std::vector<Sprite> sprite_buf;
  u16 sprite_index = 0;
  void set_ppu_mode(RENDERING_MODE mode);
  void add_sprite_to_buffer(u8 spriteIndex);
  u8 get_sprite_size() const;
  static void flip(Tile &, FLIP_AXIS);  // move to sprite class
  static void flip_cgb(Tile &, FLIP_AXIS);  // move to sprite class
  
  Tile active_tile;
  u8 window_current_y    = 0;
  u8 window_line_count   = 0;
  u8 window_x_pos_offset = 0;
  [[nodiscard]] u16 get_tile_bg_map_address_base() const;
  [[nodiscard]] u16 get_tile_window_map_address_base() const;

 public:
  Frame frame;

  u16 *disp_buf  = new u16[256 * 256];
  u16 *write_buf = new u16[256 * 256];

  DoubleBuffer db = DoubleBuffer(disp_buf, write_buf);

  SystemPalettes sys_palettes;
  RENDERING_MODE get_mode() const { return ppu_mode; }

  struct LCDC_R lcdc{0x91};
  u16 dots       = 0;
  Bus *bus       = nullptr;
  Mapper *mapper = nullptr;

  bool frame_queued = false;
  bool frame_skip   = false;

  bool hdma_active     = false;
  u16 remaining_length = 0;
  u16 hdma_index       = 0;

  bool stat_irq_fired_on_current_scanline = false;

  u8 x_pos_offset = 0;
  void tick(u16 inc);
  [[nodiscard]] std::string get_mode_string() const;

  void process_hdma_chunk();
  void increment_scanline() const;
  [[nodiscard]] Tile get_tile_data(u16 address, bool sprite = false) const;
  [[nodiscard]] Tile get_tile_sprite_data(u16 index, bool sprite = false, u8 bank = 0) const;

  const std::array<u16, 4> shade_table = {WHITE, LIGHTGREY, DARKGREY, BLACK};
  bool window_enabled                  = false;
  static std::array<Pixel, 8> decode_pixel_row(u8 high_byte, u8 low_byte);
  
  std::chrono::duration<double, std::milli> target_duration = std::chrono::duration<double, std::milli>(16.7);
  Stopwatch stopwatch;

  bool hdma_executed_on_scanline = false;
};