#include "core/ppu.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <sys/types.h>

#include <cassert>
#include <cstddef>
#include <stdexcept>

#include "common.h"

PPU::PPU(){};

void PPU::set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
void PPU::set_frame_texture(SDL_Texture* texture) {
  this->frame_texture = texture;
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

  Sprite s{sprite_y, sprite_x, tile_number, sprite_flags};
  sprite_buf.at(sprite_count++) = s;
  fmt::println("sprite count: {:d}", sprite_count);
}
void PPU::tick() {
  assert(bus != NULL);
  assert(bus->wram.data[LY] <= 153);

  if (lcdc.lcd_ppu_enable == 0) {
    return;
  }

  switch (ppu_mode) {
    case OAM_SCAN: {
      // fmt::println("dots in oam scan {:d}", dots);
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
        // fmt::println("y index: {:d}", y_index);
      }

      u16 address =
          (get_tile_map_address_base() + (y_index * 32) + ((x_pos_offset + bus->wram.data[SCX] / 8) % 32)) -
          VRAM_ADDRESS_OFFSET;

      u8 index    = bus->vram.read8(address);
      active_tile = get_tile_data(index);

      SDL_SetRenderTarget(renderer, tile_map_0);
      // HACK: tile data is being loaded in backwards; fix that!
      for (u8 x = 0; x < 8; x++) {
        switch (active_tile.pixel_data[y % 8][7 - x].color) {
          case 0: {
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
            break;
          }
          case 1: {
            // fmt::println("grey...");
            SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0x55, 0xFF);
            break;
          }
          case 2: {
            // fmt::println("grey2...");
            SDL_SetRenderDrawColor(renderer, 0xAA, 0xAA, 0xAA, 0xFF);
            break;
          }
          case 3: {
            // fmt::println("blax...");
            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
          }
          default: {
            throw std::runtime_error(fmt::format(
                "invalid color: {:#04x}", active_tile.pixel_data[y][x].color));
          }
        };
        // if (lcdc.bg_and_window_enable_priority == 0) {
        //   SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        // }
        SDL_RenderDrawPoint(renderer, x + (x_pos_offset * 8), y);
      }
      SDL_SetRenderTarget(renderer, NULL);
      x_pos_offset++;
      // fmt::println("dots in drawing {:d}", dots);
      if (x_pos_offset == 32) {
        return set_ppu_mode(HBLANK);
      }

      break;
    }
    case HBLANK: {
      // fmt::println("dots in hblank {:d}", dots);

      if (dots == 456) {
        x_pos_offset = 0;
        dots         = 0;
        sprite_index = 0;
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
      // fmt::println("dots in vblank {:d}", dots);

      if (dots == 456) {
        dots = 0;
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

    u8 f_c = h_c + l_c;
    // fmt
    pixel_array[i] = Pixel{f_c};
  }

  return pixel_array;
}
Tile PPU::get_tile_data(u8 index) {
  Tile tile;
  if (lcdc.tiles_select_method == 0) {
    for (u8 row = 0; row < 8; row++) {
      u8 low_byte  = bus->vram.read8((0x9000 + (row * 2) + ((i8)index * 16)) -
                                     VRAM_ADDRESS_OFFSET);
      u8 high_byte = bus->vram.read8(
          ((0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET) + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }
  if (lcdc.tiles_select_method == 1) {
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

u16 PPU::get_tile_map_address_base() {
  return lcdc.bg_tile_map_select ? 0x9C00 : 0x9800;
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
        bus->request_interrupt(InterruptType::VBLANK);
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