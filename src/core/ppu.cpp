#include "core/ppu.h"

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <sys/types.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>

#include "common.h"

PPU::PPU(){};

void PPU::set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
void PPU::set_frame_texture(SDL_Texture* texture) {
  this->frame_texture = texture;
}
void PPU::set_sprite_overlay_texture(SDL_Texture* texture) {
  this->sprite_overlay_texture = texture;
}
u8 PPU::get_sprite_size() { return lcdc.sprite_size == 0 ? 8 : 16; }

void PPU::add_sprite_to_buffer(u8 sprite_index) {
  u8 sprite_y     = bus->oam.data[sprite_index];
  u8 sprite_x     = bus->oam.data[sprite_index + 1];
  u8 tile_number  = bus->oam.data[sprite_index + 2];
  u8 sprite_flags = bus->oam.data[sprite_index + 3];

  this->sprite_index += 4;
  // fmt::println("sprite buf size: {:d}", sprite_buf.size());
  if (!(sprite_x > 0)) {
    return;
  }

  if (!(bus->io.data[LY] + 16 >= sprite_y)) {
    return;
  }

  if (!((bus->io.data[LY] + 16) < sprite_y + get_sprite_size())) {
    return;
  }

  if (!(sprite_buf.size() < 10)) {
    return;
  }
  Sprite current_sprite = Sprite(sprite_y, sprite_x, tile_number, sprite_flags);

  // DMG sprite prio
  // for (const auto& sprite : sprite_buf) {
  //   if (sprite.x_pos == current_sprite.x_pos) {
  //     return;
  //   }
  // }

  // CGB sprite prio

  sprite_buf.push_back(current_sprite);
}
void PPU::flip_sprite(Tile& t, FLIP_AXIS a) {
  if (a == FLIP_AXIS::X) {
    for (u8 y = 0; y < 8; y++) {
      std::reverse(std::begin(t.pixel_data[y]), std::end(t.pixel_data[y]));
    }
  } else {
    std::reverse(std::begin(t.pixel_data), std::end(t.pixel_data));
  }
}
void PPU::tick() {
  assert(bus != NULL);
  assert(bus->io.data[LY] <= 153);

  if (lcdc.lcd_ppu_enable == 0) {
    return;
  }

  switch (ppu_mode) {
    case OAM_SCAN: {
      if (bus->io.data[WY] == bus->io.data[LY] &&
          lcdc.window_disp_enable == 1) {
        window_enabled = true;
      }
      if (dots % 2 == 0 && dots != 80) {
        add_sprite_to_buffer(sprite_index);
        add_sprite_to_buffer(sprite_index);
      }
      if (dots == 80) {
        // CGB only
        std::reverse(sprite_buf.begin(), sprite_buf.end());

        assert(sprite_buf.size() < 11);
        set_ppu_mode(PIXEL_DRAW);
      }
      break;
    }
    case PIXEL_DRAW: {
      if (dots - 80 == 252) {
        set_ppu_mode(HBLANK);
      }
      break;
    }
    case HBLANK: {
      if (dots == 456) {
        x_pos_offset   = 0;
        dots           = 0;
        sprite_index   = 0;
        w_x_pos_offset = 0;

        sprite_buf.clear();

        if (window_enabled && lcdc.window_disp_enable == 1 &&
            bus->io.data[WX] <= 167) {
          w_y++;
          if (w_y % 8 == 0 && w_y != 0) {
            w_line_count++;
          }
        }
        increment_scanline();
        if (bus->io.data[LY] == 143) {
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
        dots = 0;

        sprite_buf.clear();
        return increment_scanline();
      }
      if (bus->io.data[LY] == 153) {
        set_ppu_mode(OAM_SCAN);
        bus->io.data[LY] = 0;
      }

      break;
    }
  }

  dots += 4;
}
void PPU::increment_scanline() {
  bus->io.data[LY]++;
  if (bus->io.data[LY] == bus->io.data[LYC]) {
    bus->io.data[STAT] |= (1 << 2);
  } else {
    bus->io.data[STAT] &= 0b11111011;
  }

  if ((bus->io.data[LY] == bus->io.data[LYC]) && bus->io.data[STAT] & 1 << 6) {
    return bus->request_interrupt(InterruptType::LCD);
  }
}
std::array<Pixel, 8> PPU::decode_pixel_row(u8 high_byte, u8 low_byte) {
  std::array<Pixel, 8> pixel_array;

  for (u8 i = 0; i < 8; i++) {
    u8 h_c = (high_byte & (1 << i)) != 0 ? 2 : 0;
    u8 l_c = (low_byte & (1 << i)) != 0 ? 1 : 0;

    pixel_array[i] = Pixel{static_cast<u8>(h_c + l_c)};
  }

  return pixel_array;
}

Tile PPU::get_tile_data(u16 address, bool sprite) {
  Tile tile;

  auto index = bus->vram->read8(address);

  bus->vram = &bus->vram_banks[1];

  auto matching_attr_byte = bus->vram->read8(address);

  bus->vram = &bus->vram_banks[bus->vbk];

  tile.attr_data.set_from_byte(matching_attr_byte);
  // fmt::println("fetch from vram bank: {:d}", +tile.attr_data.bank);
  bus->vram = &bus->vram_banks[tile.attr_data.bank];

  if (lcdc.tiles_select_method == 1 || sprite) {
    for (u8 row = 0; row < 8; row++) {
      u8 low_byte  = bus->vram->read8((0x8000 + (row * 2) + (index * 16)) -
                                      VRAM_ADDRESS_OFFSET);
      u8 high_byte = bus->vram->read8(
          ((0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET) + 1);

      auto pixel_array = decode_pixel_row(high_byte, low_byte);

      for (u8 pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
    bus->vram = &bus->vram_banks[bus->vbk];
    return tile;
  }
  if (lcdc.tiles_select_method == 0) {
    for (u8 row = 0; row < 8; row++) {
      u16 address =
          (0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram->read8(address);
      u8 high_byte = bus->vram->read8(address + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }

  bus->vram = &bus->vram_banks[bus->vbk];
  return tile;
}

Tile PPU::get_tile_sprite_data(u16 index, bool sprite, u8 bank) {
  Tile tile;
  assert(bank < 2);
  // auto index = bus->vram->read8(address);

  bus->vram = &bus->vram_banks[bank];

  // tile.attr_data.set_from_byte(matching_attr_byte);
  // fmt::println("fetch from vram bank: {:d}", +tile.attr_data.bank);
  // bus->vram = &bus->vram_banks[tile.attr_data.bank];

  if (lcdc.tiles_select_method == 1 || sprite) {
    for (u8 row = 0; row < 8; row++) {
      u8 low_byte  = bus->vram->read8((0x8000 + (row * 2) + (index * 16)) -
                                      VRAM_ADDRESS_OFFSET);
      u8 high_byte = bus->vram->read8(
          ((0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET) + 1);

      auto pixel_array = decode_pixel_row(high_byte, low_byte);

      for (u8 pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
    bus->vram = &bus->vram_banks[bus->vbk];
    return tile;
  }
  if (lcdc.tiles_select_method == 0) {
    for (u8 row = 0; row < 8; row++) {
      u16 address =
          (0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram->read8(address);
      u8 high_byte = bus->vram->read8(address + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }

  bus->vram = &bus->vram_banks[bus->vbk];
  return tile;
}
u16 PPU::get_tile_bg_map_address_base() {
  return lcdc.bg_tile_map_select == 1 ? 0x9C00 : 0x9800;
}
u16 PPU::get_tile_window_map_address_base() {
  return lcdc.window_tile_map_select == 1 ? 0x9C00 : 0x9800;
}

u8 PPU::get_ppu_mode() { return bus->io.data[STAT] & 0x3; };
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
  }
  assert(false);
};
void PPU::set_ppu_mode(RENDERING_MODE new_mode) {
  if (this->ppu_mode != new_mode) {
    ppu_mode = new_mode;
    bus->io.data[STAT] &= 0xfc;
    bus->io.data[STAT] |= ppu_mode;

    switch (ppu_mode) {
      case HBLANK: {
        auto mod = bus->io.data[SCX] % 8;

        u8 y = (bus->io.data[LY] + bus->io.data[SCY]);

        if (x_pos_offset != 31) {
          for (u16 z = 0; z < 31; z++) {
            if (y % 8 == 0) {
              y_index = (y / 8);
            }

            u16 address = (get_tile_bg_map_address_base() + (y_index * 32) +
                           ((x_pos_offset + (bus->io.data[SCX] / 8)) % 32)) -
                          VRAM_ADDRESS_OFFSET;

            if (window_enabled && lcdc.window_disp_enable == 1 &&
                bus->io.data[WX] <= 167 &&
                (x_pos_offset * 8) >= bus->io.data[WX] - 7) {
              address = (get_tile_window_map_address_base() +
                         (w_line_count * 32) + (w_x_pos_offset)) -
                        VRAM_ADDRESS_OFFSET;
              w_x_pos_offset++;
            }

            active_tile = get_tile_data(address);

            // Flip BG tile

            if (active_tile.attr_data.x_flip == 1) {
              flip_sprite(active_tile, FLIP_AXIS::X);
            }
            if (active_tile.attr_data.y_flip == 1) {
              flip_sprite(active_tile, FLIP_AXIS::Y);
            }

            // draw scanline
            for (u8 x = 0; x < 8; x++) {
              u32 c = sys_palettes
                          .BGP[active_tile.attr_data.color_palette]
                              [active_tile.pixel_data[y % 8][(7 - x)].color];
              i32 buf_pos =
                  x + (x_pos_offset * 8) + (bus->io.data[LY] * 256) - mod;

              // }
              // if (lcdc.bg_and_window_enable_priority == 0) {
              //   // frame.data[buf_pos] = 0xFFFFFF00;
              //   frame.data[buf_pos] = 0;
              //   frame.color_id[x + (x_pos_offset * 8) +
              //                  (bus->io.data[LY] * 256)] =
              //       active_tile.pixel_data[y % 8][7 - x].color;
              // }
              // else {
              frame.data[buf_pos] = c;
              frame
                  .color_id[x + (x_pos_offset * 8) + (bus->io.data[LY] * 256)] =
                  active_tile.pixel_data[y % 8][7 - x].color;
              frame.bg_prio[x + (x_pos_offset * 8) + (bus->io.data[LY] * 256)] =
                  active_tile.attr_data.priority;
            }
            // }

            x_pos_offset++;
          }
        }

        // Sprite
        if (lcdc.sprite_enable == 1) {
          for (u8 i = 0; i < sprite_buf.size(); i++) {
            Sprite sprite = sprite_buf[i];

            Tile top_tile;
            Tile lower_tile;

            if (get_sprite_size() == 16) {
              // fmt::println("hey");
              top_tile   = get_tile_sprite_data((sprite.tile_no & 0xFE), true,
                                                sprite.bank);
              lower_tile = get_tile_sprite_data((sprite.tile_no | 0x01), true,
                                                sprite.bank);
            } else {
              top_tile =
                  get_tile_sprite_data(sprite.tile_no, true, sprite.bank);
            }

            Palette pal = sys_palettes.get_palette_by_id(sprite.cgb_palette);

            // Flipping
            if (sprite.x_flip == 1) {
              flip_sprite(top_tile, FLIP_AXIS::X);
              flip_sprite(lower_tile, FLIP_AXIS::X);
            }
            if (sprite.y_flip == 1) {
              if (get_sprite_size() == 16) {
                std::swap(top_tile, lower_tile);
              } else {
                flip_sprite(top_tile, FLIP_AXIS::Y);
              }
            }

            u16 current_y = (bus->io.data[LY] - sprite.y_pos) + 16;
            // fmt::println("current y: {:d}", current_y);
            for (u8 x = 0; x < 8; x++) {
              u16 current_x = abs(sprite.x_pos - x - 1);
              u32 buf_pos   = (current_x) + (bus->io.data[LY] * 256);

              // if(top_tile.pixel_data[current_y % 8][x].color == 0) continue;

              // BG priority has prio over sprite prio
              if (frame.bg_prio[buf_pos] == true &&
                  frame.color_id[buf_pos] > 0 && lcdc.bg_and_window_enable_priority == 1) {
                continue;
              }

              if (sprite.obj_to_bg_priority == 0) {
                if (get_sprite_size() == 16) {
                  if (current_y < 8) {
                    if (top_tile.pixel_data[current_y % 8][x].color == 0)
                      continue;

                    frame.data[buf_pos] =
                        pal[top_tile.pixel_data[current_y % 8][x].color];
                  } else {
                    if (lower_tile.pixel_data[current_y % 8][x].color == 0)
                      continue;

                    frame.data[buf_pos] =
                        pal[lower_tile.pixel_data[current_y % 8][x].color];
                  }
                } else {
                  if (top_tile.pixel_data[current_y % 8][x].color == 0)
                    continue;

                  frame.data[buf_pos] =
                      pal[top_tile.pixel_data[current_y % 8][x].color];
                }
              }

              if (sprite.obj_to_bg_priority == 1 &&
                  frame.color_id[buf_pos] == 0) {
                
                // if (frame.bg_prio[buf_pos] == true) {
                //   continue;
                // }

                if (get_sprite_size() == 16) {
                  if (current_y < 8) {
                    if (top_tile.pixel_data[current_y % 8][x].color == 0) {
                      continue;
                    }

                    frame.data[buf_pos] =
                        pal[top_tile.pixel_data[current_y % 8][x].color];
                  } else {
                    if (lower_tile.pixel_data[current_y % 8][x].color == 0) {
                      continue;
                    }
                    frame.data[buf_pos] =
                        pal[lower_tile.pixel_data[current_y % 8][x].color];
                  }
                } else {
                  if (top_tile.pixel_data[current_y % 8][x].color == 0) {
                    continue;
                  }

                  frame.data[buf_pos] =
                      pal[top_tile.pixel_data[current_y % 8][x].color];
                }
              }
            }
          }
        }
        break;
      }
      case VBLANK: {
        // Frame d = frame;
        // Frame o = sprite_overlay;
        SDL_UpdateTexture(sprite_overlay_texture, NULL, &sprite_overlay.data,
                          2 * 256);
        SDL_UpdateTexture(frame_texture, NULL, &frame.data, 2 * 256);

        SDL_SetRenderTarget(renderer, frame_texture);
        // SDL_SetTextureBlendMode(sprite_overlay_texture, SDL_BLENDMODE_BLEND);
        // SDL_RenderCopy(renderer, sprite_overlay_texture, NULL, NULL);

        SDL_SetRenderTarget(renderer, NULL);
        bus->request_interrupt(InterruptType::VBLANK);

        sprite_overlay.clear();
        frame.clear();

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
