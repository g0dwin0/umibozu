#include "core/ppu.h"

#include <SDL2/SDL_render.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>

#include "bus.h"
#include "common.h"

// https://github.com/ocornut/imgui/issues/252
#define TOP_PADDING 7  // TODO: get rid this

// TODO: implement stat blocking

Frame::Frame() {
  data.fill(0);
}

void PPU::set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
void PPU::set_frame_texture(SDL_Texture* texture) {
  this->frame_texture = texture;
}

u8 PPU::get_sprite_size() const { return lcdc.sprite_size == 0 ? 8 : 16; }

void PPU::add_sprite_to_buffer(u8 spriteIndex) {
  u8 sprite_y     = bus->oam.data[spriteIndex];
  u8 sprite_x     = bus->oam.data[spriteIndex + 1];
  u8 tile_number  = bus->oam.data[spriteIndex + 2];
  u8 sprite_flags = bus->oam.data[spriteIndex + 3];

  this->sprite_index += 4;

  if (sprite_x <= 0) { return; }

  if (bus->io.data[LY] + 16 < sprite_y) { return; }

  if ((bus->io.data[LY] + 16) >= sprite_y + get_sprite_size()) { return; }

  if (sprite_buf.size() >= 10) { return; }
  Sprite current_sprite = Sprite(sprite_y, sprite_x, tile_number, sprite_flags);

  // DMG sprite prio
  if (bus->mode == SYSTEM_MODE::DMG) {  // refactor: this can be made into a
                                        // function, and then inlined
    for (const auto& sprite : sprite_buf) {
      if (sprite.x_pos == current_sprite.x_pos) { return; }
    }
  }
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
void PPU::tick(
    u16 dots_inc) {  // TODO: why is dots a set value, should be ticked
                     // according to the amount of dots passed.
  assert(bus != nullptr);
  assert(bus->io.data[LY] <= 153);

  if (lcdc.lcd_ppu_enable == 0) { return; }

  // fmt::println("[PPU] dots: {:d}", dots);
  switch (ppu_mode) {
    case RENDERING_MODE::OAM_SCAN: {
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
        if (bus->mode == SYSTEM_MODE::CGB) {
          std::reverse(sprite_buf.begin(), sprite_buf.end());
        }
        assert(sprite_buf.size() <= 10);
        set_ppu_mode(RENDERING_MODE::PIXEL_DRAW);
      }
      break;
    }
    case RENDERING_MODE::PIXEL_DRAW: {
      if (dots == 368) { set_ppu_mode(RENDERING_MODE::HBLANK); }
      break;
    }
    case RENDERING_MODE::HBLANK: {
      if (dots == 456) {
        x_pos_offset        = 0;
        dots                = 0;
        sprite_index        = 0;
        window_x_pos_offset = 0;

        sprite_buf.clear();

        if (window_enabled && lcdc.window_disp_enable == 1 &&
            bus->io.data[WX] < 167) {
          window_current_y++;
          if (window_current_y % 8 == 0 && window_current_y != 0) {
            window_line_count++;
          }
        }
        increment_scanline();
        if (bus->io.data[LY] == 144) {
          set_ppu_mode(RENDERING_MODE::VBLANK);
        } else {
          set_ppu_mode(RENDERING_MODE::OAM_SCAN);
        }
      }
      break;
    }
    case RENDERING_MODE::VBLANK: {
      window_enabled      = false;
      window_line_count   = 0;
      window_x_pos_offset = 0;
      window_current_y    = 0;
      if (dots == 456 && bus->io.data[LY] != 153) {
        dots = 0;

        sprite_buf.clear();
        return increment_scanline();
      }
      if (bus->io.data[LY] == 153 && dots == 456) {
        set_ppu_mode(RENDERING_MODE::OAM_SCAN);
        bus->io.data[LY] = 0;
        dots             = 0;
      }

      break;
    }
  }

  dots += dots_inc;
}
void PPU::increment_scanline() const {
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
    pixel_array[i] =
        Pixel{.color = static_cast<u8>(((high_byte & (1 << i)) != 0 ? 2 : 0) +
                                       ((low_byte & (1 << i)) != 0 ? 1 : 0))};
  }

  return pixel_array;
}

Tile PPU::get_tile_data(u16 address, bool sprite) const {
  Tile tile;

  u8 index = bus->vram->read8(address);
  u8 matching_attr_byte;
  if (bus->mode == SYSTEM_MODE::CGB) {
    // bus->vram = &bus->vram_banks[1];

    matching_attr_byte = bus->vram_banks[1].read8(address);

    bus->vram = &bus->vram_banks[bus->vbk];

    tile.attr_data.value = (matching_attr_byte);
    bus->vram            = &bus->vram_banks[tile.attr_data.bank];
  }

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
    if (bus->mode == SYSTEM_MODE::CGB) {
      bus->vram = &bus->vram_banks[bus->vbk];
    }
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
  if (bus->mode == SYSTEM_MODE::CGB) { bus->vram = &bus->vram_banks[bus->vbk]; }
  return tile;
}

Tile PPU::get_tile_sprite_data(u16 index, bool sprite, u8 bank) const {
  Tile tile;
  assert(bank < 2);

  if (bus->mode == SYSTEM_MODE::CGB) { bus->vram = &bus->vram_banks[bank]; }

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
    if (bus->mode == SYSTEM_MODE::CGB) {
      bus->vram = &bus->vram_banks[bus->vbk];
    }
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
  if (bus->mode == SYSTEM_MODE::CGB) { bus->vram = &bus->vram_banks[bus->vbk]; }
  return tile;
}
u16 PPU::get_tile_bg_map_address_base() const {
  return lcdc.bg_tile_map_select == 1 ? 0x9C00 : 0x9800;
}
u16 PPU::get_tile_window_map_address_base() const {
  return lcdc.window_tile_map_select == 1 ? 0x9C00 : 0x9800;
}

std::string PPU::get_mode_string() {
  switch (get_mode()) {
    case RENDERING_MODE::HBLANK: {
      return "HBLANK (mode 0)";
    }
    case RENDERING_MODE::VBLANK: {
      return "VBLANK (mode 1)";
    }
    case RENDERING_MODE::OAM_SCAN: {
      return "OAM SCAN (mode 2)";
    }
    case RENDERING_MODE::PIXEL_DRAW: {
      return "DRAWING (mode 3)";
    }
  }
  assert(false);
}

void PPU::set_ppu_mode(RENDERING_MODE new_mode) {
  if (this->ppu_mode != new_mode) {
    ppu_mode = new_mode;
    bus->io.data[STAT] &= 0xfc;
    bus->io.data[STAT] |= (u8)ppu_mode;

    switch (ppu_mode) {
      case RENDERING_MODE::HBLANK: {
        // Process HDMA transferF

        // u8 mod = bus->io.data[SCX] % 8;

        u8 y = (bus->io.data[LY] + bus->io.data[SCY]);

        for (u16 scanline_tile_index = 0; scanline_tile_index < 32;
             scanline_tile_index++) {
          // if (y % 8 == 0) {
          //   y_index = (y / 8);
          // }

          u16 address = (get_tile_bg_map_address_base() + ((y / 8) * 32) +
                         ((x_pos_offset + (bus->io.data[SCX] / 8)) % 32)) -
                        VRAM_ADDRESS_OFFSET;

          if (window_enabled && lcdc.window_disp_enable == 1 &&
              bus->io.data[WX] < 167 &&
              (x_pos_offset * 8) >= bus->io.data[WX] - 7) {
            address = (get_tile_window_map_address_base() +
                       (window_line_count * 32) + (window_x_pos_offset)) -
                      VRAM_ADDRESS_OFFSET;
            window_x_pos_offset++;
          }

          active_tile = get_tile_data(address);

          // Flip BG tile

          if (active_tile.attr_data.x_flip == 1) {
            flip_sprite(active_tile, FLIP_AXIS::X);
          }
          if (active_tile.attr_data.y_flip == 1) {
            flip_sprite(active_tile, FLIP_AXIS::Y);
          }

          for (u8 x = 0; x < 8; x++) {
            u16 color;

            if (bus->mode == SYSTEM_MODE::CGB) {
              color = sys_palettes
                          .BGP[active_tile.attr_data.color_palette]
                              [active_tile.pixel_data[y % 8][(7 - x)].color];
            } else {
              color = sys_palettes
                          .BGP[0][active_tile.pixel_data[y % 8][(7 - x)].color];
            }
            u32 line_base = ((bus->io.data[LY] + TOP_PADDING) * 256);

            i32 line_x;

            if (window_enabled && lcdc.window_disp_enable == 1 &&
                bus->io.data[WX] < 167 &&
                x + (x_pos_offset * 8) >= bus->io.data[WX] - 7) {
              line_x = abs(x + (x_pos_offset * 8));
            } else {
              line_x = abs(x + (x_pos_offset * 8) - (bus->io.data[SCX] & 7));
            }

            u32 buf_pos = line_base + line_x;

            if (bus->mode == SYSTEM_MODE::DMG) {
              if (lcdc.bg_and_window_enable_priority == 0) {
                frame.data[buf_pos]     = WHITE;
                frame.color_id[buf_pos] = 0;

                continue;
              }
            }

            frame.data[buf_pos] = color;
            frame.color_id[buf_pos] =
                active_tile.pixel_data[y % 8][7 - x].color;
            frame.bg_prio[buf_pos] = active_tile.attr_data.priority;
          }

          x_pos_offset++;
        }

        // Sprite
        if (lcdc.sprite_enable == 1) {
          for (auto sprite : sprite_buf) {
            Tile top_tile;
            Tile lower_tile;

            if (get_sprite_size() == 16) {
              top_tile   = get_tile_sprite_data((sprite.tile_no & 0xFE), true,
                                                sprite.bank);
              lower_tile = get_tile_sprite_data((sprite.tile_no | 0x01), true,
                                                sprite.bank);
            } else {
              top_tile =
                  get_tile_sprite_data(sprite.tile_no, true, sprite.bank);
            }

            Palette pal;

            if (bus->mode == SYSTEM_MODE::CGB) {
              pal = sys_palettes.get_palette_by_id(sprite.cgb_palette);
            } else {
              pal = sys_palettes.get_palette_by_id(sprite.palette_number);
            }

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
            // fmt::println("[PPU] current y: {:d}", current_y);
            for (u8 x = 0; x < 8; x++) {
              // u16 current_x = abs(sprite.x_pos - x - 1);
              i16 current_x = sprite.x_pos - x - 1;
              if (current_x < 0) { continue; }

              u32 buf_pos =
                  (current_x) + ((bus->io.data[LY] + TOP_PADDING) * 256);

              // if(top_tile.pixel_data[current_y % 8][x].color == 0) continue;

              if (bus->mode == SYSTEM_MODE::CGB &&
                  (bus->io.data[OPRI] & 0x1) == (u8)PRIORITY_MODE::CGB) {
                // BG priority has prio over sprite prio
                if (lcdc.bg_and_window_enable_priority &&
                    (sprite.obj_to_bg_priority || frame.bg_prio[buf_pos]) &&
                    frame.color_id[buf_pos] > 0) {
                  continue;
                }
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

        // if ((bus->io.data[HDMA5] & 0x80) == 0 &&
        //     bus->mode == COMPAT_MODE::CGB_ONLY) {
        //   u16 src = ((bus->io.data[HDMA1] << 8) + bus->io.data[HDMA2]) &
        //   0xfff0; u16 dst = ((bus->io.data[HDMA3] << 8) +
        //   bus->io.data[HDMA4]) & 0x1ff0; for (size_t index = 0; index < 0x10;
        //   index++) {
        //     u16 src_address = (src + (hdma_index * 0x10) + index);
        //     u8 src_data     = mapper->read8(src_address);

        //     u16 vram_address = ((dst + (hdma_index * 0x10) + index));
        //     if (vram_address > 0x1FFF) {
        //       bus->io.data[HDMA5] |= 0x80;
        //       fmt::println("[PPU] overflow");
        //       // assert(false && "overflow");
        //     } else {
        //       // fmt::println(
        //       //     "[HDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: "
        //       //     "{:#16x} ",
        //       //     src_address, src_data, 0x8000 + vram_address);

        //       bus->vram->write8(vram_address, src_data);
        //     }
        //   }
        //   if ((bus->io.data[HDMA5] & 0x7f) == 0) {
        //     // we're done
        //     fmt::println("[PPU] TRANSFER FINISHNED");
        //     bus->io.data[HDMA5] = 0xFF;
        //     hdma_index          = 0;
        //   } else {
        //     hdma_index++;
        //     u8 remaining_blocks = bus->io.data[HDMA5] & 0x7f;
        //     remaining_blocks--;
        //     bus->io.data[HDMA5] = remaining_blocks;

        //   }
        // }
        break;
      }

      case RENDERING_MODE::VBLANK: {
        SDL_UpdateTexture(frame_texture, nullptr, &frame.data, 2 * 256);
        bus->request_interrupt(InterruptType::VBLANK);

        frame.clear();
        if (frame_skip) {
          frame_skip = false;
          // fmt::println("[PPU] frame skipped after re-enabling");
          break;
        }
        frame_queued = true;
        break;
      }
      case RENDERING_MODE::OAM_SCAN: {
        break;
      }
      case RENDERING_MODE::PIXEL_DRAW: {
        break;
      }
    }
  }
}

SystemPalettes::SystemPalettes() {
  BGP[0] = {WHITE, LIGHTGREY, DARKGREY, BLACK};
}

Palette SystemPalettes::get_palette_by_id(u8 index) {
  assert(index < 8);
  return OBP[index];
}