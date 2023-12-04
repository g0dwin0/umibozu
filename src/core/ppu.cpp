#include "core/ppu.h"

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <sys/types.h>

#include <cassert>
#include <cstddef>
#include <stdexcept>

#include "common.h"
#include "fmt/core.h"

PPU::PPU(){};

void PPU::set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
void PPU::set_frame_texture(SDL_Texture* texture) {
  this->frame_texture = texture;
}
void PPU::set_sprite_overlay_texture(SDL_Texture* texture) {
  this->sprite_overlay_texture = texture;
}

void PPU::add_sprite_to_buffer(u8 sprite_index) {
  u8 sprite_y     = bus->oam.data[sprite_index];
  u8 sprite_x     = bus->oam.data[sprite_index + 1];
  u8 tile_number  = bus->oam.data[sprite_index + 2];
  u8 sprite_flags = bus->oam.data[sprite_index + 3];

  this->sprite_index += 4;
  if (!(sprite_x > 0)) {
    return;
  }

  if (!(bus->wram.data[LY] + 16 >= sprite_y)) {
    return;
  }

  if (!(bus->wram.data[LY] + 16 <
        sprite_y + (lcdc.sprite_size == 1 ? 16 : 8))) {
    return;
  }

  if (!(sprite_count < 10)) {
    return;
  }
  // fmt::println("LY: {:d}", bus->wram.data[LY]);
  Sprite s = Sprite(sprite_y, sprite_x, tile_number, sprite_flags);

  sprite_buf.at(sprite_count++) = s;
  // fmt::println("sprite count: {:d}", sprite_count);
}
Tile PPU::flip_sprite(Tile t, FLIP_AXIS a) {
  Tile n;

  if (a == FLIP_AXIS::X) {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        n.pixel_data[y][x] = t.pixel_data[y][7 - x];
      }
    }
  } else {
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        n.pixel_data[y][x] = t.pixel_data[7 - y][x];
      }
    }
  }

  return n;
}
void PPU::tick() {
  assert(bus != NULL);
  assert(bus->wram.data[LY] <= 153);

  if (lcdc.lcd_ppu_enable == 0) {
    return;
  }

  switch (ppu_mode) {
    case OAM_SCAN: {
      if (bus->wram.data[WY] == bus->wram.data[LY] &&
          lcdc.window_disp_enable == 1) {
        window_enabled = true;
      }
      if (dots % 2 == 0) {
        add_sprite_to_buffer(sprite_index);
      }
      if (dots == 80) {
        set_ppu_mode(PIXEL_DRAW);
      }
      break;
    }
    case PIXEL_DRAW: {
      // draw code
      u8 y = bus->wram.data[LY] + bus->wram.data[SCY];

      if (y % 8 == 0) {
        y_index = (y / 8);
      }

      u16 address = (get_tile_bg_map_address_base() + (y_index * 32) +
                     ((x_pos_offset + bus->wram.data[SCX] / 8) % 32)) -
                    VRAM_ADDRESS_OFFSET;

      if (window_enabled && lcdc.window_disp_enable == 1 &&
          bus->wram.data[WX] <= 167 &&
          (x_pos_offset * 8) >= bus->wram.data[WX] - 7) {
        address = (get_tile_window_map_address_base() + (w_line_count * 32) +
                   (w_x_pos_offset)) -
                  VRAM_ADDRESS_OFFSET;
        w_x_pos_offset++;
      }

      u8 index    = bus->vram.read8(address);
      active_tile = get_tile_data(index);

      // HACK: tile data is being loaded in backwards; fix that!
      // TODO: decouple frontend-reliant drawing code, use frame struct

      for (u8 x = 0; x < 8; x++) {
        u32 c = BGP[active_tile.pixel_data[y % 8][7 - x].color];

        if (lcdc.bg_and_window_enable_priority == 0) {
          frame.data[x + (x_pos_offset * 8) + (y * 256)] = 0xFFFFFF00;
          frame.color_id[x + (x_pos_offset * 8) + (y * 256)] =
              active_tile.pixel_data[y % 8][7 - x].color;
        } else {
          frame.data[x + (x_pos_offset * 8) + (y * 256)] = c;
          frame.color_id[x + (x_pos_offset * 8) + (y * 256)] =
              active_tile.pixel_data[y % 8][7 - x].color;
        }
      }

      for (u8 i = 0; i < sprite_count; i++) {
        Sprite sprite = sprite_buf[i];
        for (u8 x = 0; x < 8; x++) {
          if (sprite.x_pos >= (x_pos_offset * 8) + x &&
              sprite.x_pos <= (x_pos_offset * 8) + 8) {
            Tile t = get_tile_data(sprite.tile_no, true);

            if (sprite.x_flip == 1)
              t = flip_sprite(t, FLIP_AXIS::X);
            if (sprite.y_flip == 1)
              t = flip_sprite(t, FLIP_AXIS::Y);

            if (sprite.palette_number == 0) {
              if (sprite.obj_to_bg_priority == 0) {
                sprite_overlay.data[x + (x_pos_offset * 8) + (y * 256)] =
                    OBP_0[t.pixel_data[y % 8][7 - x].color];
              }

              if (sprite.obj_to_bg_priority == 1 &&
                  frame.color_id[x + (x_pos_offset * 8) + (y * 256)] == 0) {
                sprite_overlay.data[x + (x_pos_offset * 8) + (y * 256)] =
                    OBP_0[t.pixel_data[y % 8][7 - x].color];
              }

            } else {
              if (sprite.obj_to_bg_priority == 0) {
                sprite_overlay.data[x + (x_pos_offset * 8) + (y * 256)] =
                    OBP_1[t.pixel_data[y % 8][7 - x].color];
              }

              if (sprite.obj_to_bg_priority == 1 &&
                  frame.color_id[x + (x_pos_offset * 8) + (y * 256)] == 0) {
                sprite_overlay.data[x + (x_pos_offset * 8) + (y * 256)] =
                    OBP_1[t.pixel_data[y % 8][7 - x].color];
              }
            }
          }
        }
      }

      x_pos_offset++;

      if (x_pos_offset == 32) {
        return set_ppu_mode(HBLANK);
      }

      break;
    }
    case HBLANK: {
      if (dots == 456) {
        x_pos_offset   = 0;
        dots           = 0;
        sprite_index   = 0;
        w_x_pos_offset = 0;
        sprite_count   = 0;

        if (window_enabled && lcdc.window_disp_enable == 1 &&
            bus->wram.data[WX] <= 167) {
          w_y++;
          if (w_y % 8 == 0 && w_y != 0) {
            w_line_count++;
          }
        }
        increment_scanline();
        if (bus->wram.data[LY] == 143) {
          set_ppu_mode(VBLANK);
        } else {
          set_ppu_mode(OAM_SCAN);
        }
      }
      break;
    }
    case VBLANK: {
      window_enabled = false;
      w_line_count   = 0;
      w_y            = 0;
      if (dots == 456) {
        dots         = 0;
        sprite_count = 0;
        return increment_scanline();
      }

      if (bus->wram.data[LY] == 153) {
        set_ppu_mode(OAM_SCAN);
        bus->wram.data[LY] = 0;
      }
      break;
    }
  }

  dots += 4;
}
void PPU::increment_scanline() {
  bus->wram.data[LY]++;
  if (bus->wram.data[LY] == bus->wram.data[LYC]) {
    bus->wram.data[STAT] |= (1 << 2);
  } else {
    bus->wram.data[STAT] &= 0b11111011;
  }

  if ((bus->wram.data[LY] == bus->wram.data[LYC]) &&
      bus->wram.data[STAT] & 1 << 6) {
    return bus->request_interrupt(InterruptType::LCD);
  }
}
std::array<Pixel, 8> PPU::decode_pixel_row(u8 high_byte, u8 low_byte) {
  std::array<Pixel, 8> pixel_array;

  for (int i = 0; i < 8; i++) {
    u8 h_c = (high_byte & (1 << i)) != 0 ? 2 : 0;
    u8 l_c = (low_byte & (1 << i)) != 0 ? 1 : 0;

    u8 f_c         = h_c + l_c;
    pixel_array[i] = Pixel{f_c};
  }

  return pixel_array;
}

Tile PPU::get_tile_data(u8 index, bool sprite) {
  Tile tile;
  if (lcdc.tiles_select_method == 0) {
    for (u8 row = 0; row < 8; row++) {
      u16 address =
          (0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram.read8(address);
      u8 high_byte = bus->vram.read8(address + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }
  if (lcdc.tiles_select_method == 1 || sprite) {
    for (u8 row = 0; row < 8; row++) {
      u8 low_byte  = bus->vram.read8((0x8000 + (row * 2) + (index * 16)) -
                                     VRAM_ADDRESS_OFFSET);
      u8 high_byte = bus->vram.read8(
          ((0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET) + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }
  return tile;
}

u16 PPU::get_tile_bg_map_address_base() {
  return lcdc.bg_tile_map_select == 1 ? 0x9C00 : 0x9800;
}
u16 PPU::get_tile_window_map_address_base() {
  return lcdc.window_tile_map_select == 1 ? 0x9C00 : 0x9800;
}

u8 PPU::get_ppu_mode() { return bus->wram.data[STAT] & 0x3; };
std::string PPU::get_mode_string() {
  switch (get_mode()) {
    case HBLANK: {
      return "HBLANK (mode 0)";
    }
    case VBLANK: {
      return "VBLANK (mode 1)";
    }
    case OAM_SCAN: {
      return "OAM SCAN (mode 2)";
    }
    case PIXEL_DRAW: {
      return "DRAWING (mode 3)";
    }
    default: {
      throw std::runtime_error("invalid rendering mode");
    }
  }
};
void PPU::set_ppu_mode(RENDERING_MODE new_mode) {
  if (this->ppu_mode != new_mode) {
    ppu_mode = new_mode;
    bus->wram.data[STAT] &= 0xfc;
    bus->wram.data[STAT] |= ppu_mode;

    switch (ppu_mode) {
      case HBLANK: {
        break;
      }
      case VBLANK: {
        SDL_UpdateTexture(sprite_overlay_texture, NULL, &sprite_overlay.data,
                          4 * 256);
        SDL_UpdateTexture(frame_texture, NULL, &frame.data, 4 * 256);

        SDL_SetRenderTarget(renderer, frame_texture);
        SDL_SetTextureBlendMode(sprite_overlay_texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, sprite_overlay_texture, NULL, NULL);

        SDL_SetRenderTarget(renderer, NULL);
        bus->request_interrupt(InterruptType::VBLANK);

        sprite_overlay = Frame();
        frame          = Frame();

        frame_queued = true;
        break;
      }
      case OAM_SCAN: {
        break;
      }
      case PIXEL_DRAW: {
        break;
      }
    }
  }
};