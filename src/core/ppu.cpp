#include "core/ppu.hpp"

#include <SDL2/SDL_render.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iterator>
#include <thread>

#include "bus.hpp"
#include "common.hpp"
#include "stopwatch.hpp"

Frame::Frame() {};

u8 PPU::get_sprite_size() const { return lcdc.sprite_size == 0 ? 8 : 16; }

void PPU::add_sprite_to_buffer(u8 spriteIndex) {
  u8 sprite_y     = bus->oam.at(spriteIndex);
  u8 sprite_x     = bus->oam.at(spriteIndex + 1);
  u8 tile_number  = bus->oam.at(spriteIndex + 2);
  u8 sprite_flags = bus->oam.at(spriteIndex + 3);

  this->sprite_index += 4;

  // fmt::println("==== sprite index: {} ====", (spriteIndex/4));
  // fmt::println("sprite_y: {}", bus->oam.at(spriteIndex));
  // fmt::println("sprite_x: {}", bus->oam.at(spriteIndex + 1));
  // fmt::println("tile_number: {}", bus->oam.at(spriteIndex + 2));
  // fmt::println("sprite_flags: {}", bus->oam.at(spriteIndex + 3));
  // fmt::println("==================================");

  // Check if the sprite is to be rendered on the current scanline

  if (!(sprite_x > 0)) {
    return;
  }

  if (!((bus->io[LY] + 16) >= sprite_y)) {
    return;
  }

  if (!((bus->io[LY] + 16) < sprite_y + get_sprite_size())) {
    return;
  }

  if (sprite_buf.size() >= 10) {
    return;
  }

  assert(sprite_buf.size() <= 10);

  Sprite current_sprite = Sprite(sprite_y, sprite_x, tile_number, sprite_flags);

  sprite_buf.push_back(current_sprite);
}

void PPU::flip(Tile& t, FLIP_AXIS a) {
  if (a == FLIP_AXIS::X) {
    for (u8 y = 0; y < 8; y++) {
      std::reverse(std::begin(t.pixel_data[y]), std::end(t.pixel_data[y]));
    }
  } else {
    std::reverse(std::begin(t.pixel_data), std::end(t.pixel_data));
  }
}

void PPU::tick(u16 dots_inc) {
  assert(bus != nullptr);
  assert(bus->io[LY] <= 153);

  // Order: [Mode 2 / OAM scan] -> [Mode 3 / Drawing] -> [Mode 0 / HBLANK]
  // The actual drawing happens in HBLANK, we do nothing in mode 3,
  // Mode 1, is VBLANK, we do nothing but count up until we hit scanline 160~
  // then we start over

  if (!lcdc.lcd_ppu_enable) {
    return;
  }

  switch (ppu_mode) {
    case RENDERING_MODE::OAM_SCAN: {
      if ((bus->io[WY] == bus->io[LY]) && lcdc.window_disp_enable) {
        window_enabled = true;
      }

      if (dots % 2 == 0 && dots != 80) {
        add_sprite_to_buffer(sprite_index);
        add_sprite_to_buffer(sprite_index);
      }

      if (dots == 80) {
        if (bus->mode == SYSTEM_MODE::CGB) {
          // std::reverse(sprite_buf.begin(), sprite_buf.end());
          std::sort(sprite_buf.begin(), sprite_buf.end(), [](const Sprite& sprite0, const Sprite& sprite1) { return sprite0.x_pos < sprite1.x_pos; });
        }

        if (bus->mode == SYSTEM_MODE::DMG || bus->io[OPRI] == PRIORITY_MODE::DMG) {
          std::stable_sort(sprite_buf.begin(), sprite_buf.end(), [](const Sprite& sprite0, const Sprite& sprite1) { return sprite0.x_pos >= sprite1.x_pos; });
        }

        assert(sprite_buf.size() <= 10);
        set_ppu_mode(RENDERING_MODE::PIXEL_DRAW);
      }

      break;
    }

    case RENDERING_MODE::PIXEL_DRAW: {
      if (dots == 368) {
        set_ppu_mode(RENDERING_MODE::HBLANK);
      }
      break;
    }

    case RENDERING_MODE::HBLANK: {
      if ((bus->io[HDMA5] & 0x80) == 0 && bus->mode == SYSTEM_MODE::CGB && !bus->cpu_is_halted && !hdma_executed_on_scanline) {
        assert(bus->io[LY] <= 144);

        u16 src = ((bus->io[HDMA1] << 8) + bus->io[HDMA2]) & 0xfff0;
        u16 dst = ((bus->io[HDMA3] << 8) + bus->io[HDMA4]) & 0x1ff0;

        for (size_t index = 0; index < 0x10; index++) {
          u16 src_address = (src + (hdma_index * 0x10) + index);
          u8 src_data     = mapper->read8(src_address);

          u16 vram_address = ((dst + (hdma_index * 0x10) + index));

          if (vram_address > 0x1FFF) {
            bus->io[HDMA5] |= 0x80;
            // fmt::println("[PPU] overflow");
            // assert(false && "overflow");
          } else {
            // fmt::println(
            //     "[HDMA] addr: {:#16x} data: {:#04x} -> VRAM ADDRESS: "
            //     "{:#16x} ",
            //     src_address, src_data, 0x8000 + vram_address);

            // fmt::println("{:#010x}", 0x8000 + vram_address);
            bus->vram->at(vram_address) = src_data;
            // fmt::println("{:#010x} = {:#02X}", 0x8000 + vram_address, bus->vram->at(vram_address));
          }
        }

        if ((bus->io[HDMA5] & 0x7f) == 0) {
          // we're done
          fmt::println("[PPU] TRANSFER FINISHNED");
          bus->io[HDMA5] = 0xFF;
          hdma_index     = 0;
        } else {
          hdma_index++;
          u8 remaining_blocks = bus->io[HDMA5] & 0x7f;
          remaining_blocks--;
          bus->io[HDMA5] = remaining_blocks;
          fmt::println("remaining blocks: {}", remaining_blocks);
        }
        hdma_executed_on_scanline = true;
      }

      if (dots == 456) {
        x_pos_offset        = 0;
        dots                = 0;
        sprite_index        = 0;
        window_x_pos_offset = 0;

        sprite_buf.clear();

        if (window_enabled && lcdc.window_disp_enable == 1 && bus->io[WX] < 167) {
          window_current_y++;
          if (window_current_y % 8 == 0 && window_current_y != 0) {
            window_line_count++;
          }
        }

        increment_scanline();
        hdma_executed_on_scanline = false;
        if (bus->io[LY] == 144) {
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

      // Increment scanline, but we're still in VBLANK
      if (dots == 456 && bus->io[LY] != 153) {
        dots = 0;

        sprite_buf.clear();
        increment_scanline();
        return;
      }

      // Reset scanline to line 0, start drawing new frame
      if (bus->io[LY] == 153 && dots == 456) {
        set_ppu_mode(RENDERING_MODE::OAM_SCAN);
        bus->io[LY] = 0;
        dots        = 0;
        stopwatch.start();
      }

      break;
    }
  }

  dots += dots_inc;
}
void PPU::increment_scanline() const {
  bool old_hidden_stat = bus->hidden_stat;

  bus->io.at(LY) = bus->io.at(LY) + 1;

  bus->update_hidden_stat();

  if (old_hidden_stat == 0 && bus->hidden_stat == 1) {
    bus->request_interrupt(INTERRUPT_TYPE::LCD);
  }
}

std::array<Pixel, 8> PPU::decode_pixel_row(u8 high_byte, u8 low_byte) {
  std::array<Pixel, 8> pixel_array;

  for (u8 i = 0; i < 8; i++) {
    pixel_array[i] = static_cast<u8>(((high_byte & (1 << i)) != 0 ? 2 : 0) + ((low_byte & (1 << i)) != 0 ? 1 : 0));
  }

  return pixel_array;
}

Tile PPU::get_tile_data(u16 address, bool sprite) const {
  Tile tile;

  u8 index = bus->vram->at(address);
  u8 matching_attr_byte;
  if (bus->mode == SYSTEM_MODE::CGB) {
    matching_attr_byte = bus->vram_banks[1].at(address);

    bus->vram = &bus->vram_banks[bus->vbk];

    tile.cgb_attr_data.value = matching_attr_byte;
    bus->vram                = &bus->vram_banks[tile.cgb_attr_data.bank];
  }

  if (lcdc.tiles_select_method == 1 || sprite) {
    for (u8 row = 0; row < 8; row++) {
      u8 low_byte  = bus->vram->at((0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET);
      u8 high_byte = bus->vram->at(((0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET) + 1);

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
      u16 address  = (0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram->at(address);
      u8 high_byte = bus->vram->at(address + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (int pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }
  }

  if (bus->mode == SYSTEM_MODE::CGB) {
    bus->vram = &bus->vram_banks[bus->vbk];
  }

  return tile;
}

Tile PPU::get_tile_sprite_data(u16 index, bool sprite, u8 bank) const {
  Tile tile;

  assert(bank < 2);

  if (lcdc.tiles_select_method == 1 || sprite) {
    if (bus->mode == SYSTEM_MODE::CGB) {
      bus->vram = &bus->vram_banks[bank];
    }

    for (u8 row = 0; row < 8; row++) {
      u16 addr     = (0x8000 + (row * 2) + (index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram->at(addr);
      u8 high_byte = bus->vram->at(addr + 1);

      auto pixel_array = decode_pixel_row(high_byte, low_byte);

      for (u8 pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }

    if (bus->mode == SYSTEM_MODE::CGB) {
      bus->vram = &bus->vram_banks[bus->vbk];
    }

    return tile;
  } else {
    for (u8 row = 0; row < 8; row++) {
      u16 address  = (0x9000 + (row * 2) + ((i8)index * 16)) - VRAM_ADDRESS_OFFSET;
      u8 low_byte  = bus->vram->at(address);
      u8 high_byte = bus->vram->at(address + 1);

      const auto& pixel_array = decode_pixel_row(high_byte, low_byte);

      for (u8 pixel_index = 0; pixel_index < 8; pixel_index++) {
        tile.pixel_data[row][pixel_index] = pixel_array[pixel_index];
      }
    }

    if (bus->mode == SYSTEM_MODE::CGB) {
      bus->vram = &bus->vram_banks[bus->vbk];
    }

    return tile;
  }
}

u16 PPU::get_tile_bg_map_address_base() const { return lcdc.bg_tile_map_select == 1 ? 0x9C00 : 0x9800; }
u16 PPU::get_tile_window_map_address_base() const { return lcdc.window_tile_map_select == 1 ? 0x9C00 : 0x9800; }

std::string PPU::get_mode_string() const {
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

    bool old_hidden_stat = bus->hidden_stat;

    bus->io[STAT] &= 0xfc;
    bus->io[STAT] |= (u8)ppu_mode;

    bus->update_hidden_stat();

    if (old_hidden_stat == 0 && bus->hidden_stat == 1) {
      bus->request_interrupt(INTERRUPT_TYPE::LCD);
    }

    switch (ppu_mode) {
      case RENDERING_MODE::HBLANK: {
        u8 y = bus->io[LY] + bus->io[SCY];

        for (u16 scanline_tile_index = 0; scanline_tile_index < 32; scanline_tile_index++) {  // A scanline is 256 pixels long.

          u16 address = (get_tile_bg_map_address_base() + ((y / 8) * 32) + ((x_pos_offset + (bus->io[SCX] / 8)) % 32)) - VRAM_ADDRESS_OFFSET;

          if (window_enabled && lcdc.window_disp_enable && bus->io[WX] < 167 && (x_pos_offset * 8) >= bus->io[WX] - 7) {
            address = (get_tile_window_map_address_base() + (window_line_count * 32) + (window_x_pos_offset)) - VRAM_ADDRESS_OFFSET;
            window_x_pos_offset++;
          }

          active_tile = get_tile_data(address);

          if (active_tile.cgb_attr_data.x_flip) {
            flip(active_tile, FLIP_AXIS::X);
          }
          if (active_tile.cgb_attr_data.y_flip) {
            flip(active_tile, FLIP_AXIS::Y);
          }

          for (u8 x = 0; x < 8; x++) {
            u16 color;

            if (bus->mode == SYSTEM_MODE::CGB) {
              color = sys_palettes.BGP[active_tile.cgb_attr_data.color_palette][active_tile.pixel_data[y % 8][(7 - x)]];
            } else {
              color = sys_palettes.BGP[0][active_tile.pixel_data[y % 8][(7 - x)]];
            }

            u32 line_base = ((bus->io[LY]) * 256);

            i32 line_x;

            if (window_enabled && lcdc.window_disp_enable && bus->io[WX] < 167 && x + (x_pos_offset * 8) >= bus->io[WX] - 7) {
              line_x = abs(x + (x_pos_offset * 8));
            } else {
              line_x = abs(x + (x_pos_offset * 8) - (bus->io[SCX] & 7));
            }

            u32 buf_pos = line_base + line_x;

            if (bus->mode == SYSTEM_MODE::DMG && !lcdc.bg_and_window_enable_priority) {
              // frame.data[buf_pos]     = WHITE;
              frame.color_id[buf_pos] = 0;
              db.write(buf_pos, WHITE);
              continue;
            }

            // frame.data[buf_pos]     = color;

            // TODO: holds color_id, not mapped to bgr color
            // used to determine priority
            frame.color_id[buf_pos] = active_tile.pixel_data[y % 8][7 - x];
            frame.bg_prio[buf_pos]  = active_tile.cgb_attr_data.priority;
            db.write(buf_pos, color);
          }

          x_pos_offset++;
        }

        if (lcdc.sprite_enable == 1) {
          for (const Sprite& sprite : sprite_buf) {
            Palette palette;
            Tile top_tile;
            Tile lower_tile;

            if (get_sprite_size() == 16) {
              top_tile   = get_tile_sprite_data((sprite.tile_no & 0xFE), true, sprite.bank);
              lower_tile = get_tile_sprite_data((sprite.tile_no | 0x01), true, sprite.bank);
            } else {
              top_tile = get_tile_sprite_data(sprite.tile_no, true, sprite.bank);
            }

            if (bus->mode == SYSTEM_MODE::CGB) {
              palette = sys_palettes.get_palette_by_id(sprite.cgb_palette);
            } else {
              palette = sys_palettes.get_palette_by_id(sprite.palette_number);
            }

            if (sprite.x_flip == 1) {
              flip(top_tile, FLIP_AXIS::X);
              flip(lower_tile, FLIP_AXIS::X);
            }

            if (sprite.y_flip == 1) {
              if (get_sprite_size() == 16) {
                std::swap(top_tile, lower_tile);
              } else {
                flip(top_tile, FLIP_AXIS::Y);
              }
            }

            u16 current_y = (bus->io[LY] - sprite.y_pos) + 16;

            for (u8 x = 0; x < 8; x++) {
              i16 current_x = sprite.x_pos - x - 1;
              if (current_x < 0) {
                continue;
              }

              u32 buf_pos = (current_x) + ((bus->io[LY]) * 256);

              if (bus->mode == SYSTEM_MODE::CGB) {
                // https://gbdev.io/pandocs/Tile_Maps.html#bg-to-obj-priority-in-cgb-mode
                if (lcdc.bg_and_window_enable_priority && (sprite.obj_to_bg_priority || frame.bg_prio[buf_pos]) && frame.color_id[buf_pos] > 0) {
                  continue;
                }
              }

              if (get_sprite_size() == 16) {
                if (current_y < 8) {
                  if (top_tile.pixel_data[current_y % 8][x] == 0) continue;

                  db.write(buf_pos, palette[top_tile.pixel_data[current_y % 8][x]]);
                } else {
                  if (lower_tile.pixel_data[current_y % 8][x] == 0) continue;

                  db.write(buf_pos, palette[lower_tile.pixel_data[current_y % 8][x]]);
                }
              } else {
                if (top_tile.pixel_data[current_y % 8][x] == 0) continue;

                db.write(buf_pos, palette[top_tile.pixel_data[current_y % 8][x]]);
              }
            }
          }
        }
        break;
      }

      case RENDERING_MODE::VBLANK: {
        bus->request_interrupt(INTERRUPT_TYPE::VBLANK);
        db.swap_buffers();
        frame.bg_prio.fill(false);
        frame.color_id.fill(0);
        stopwatch.end();

        auto target_duration = std::chrono::duration<double, std::milli>(16.7);
        std::this_thread::sleep_for(target_duration - stopwatch.duration);

        break;
      }

      case RENDERING_MODE::OAM_SCAN:
      case RENDERING_MODE::PIXEL_DRAW: {
        break;
      }
    }
  }
}

SystemPalettes::SystemPalettes() { BGP[0] = {WHITE, LIGHTGREY, DARKGREY, BLACK}; }

Palette SystemPalettes::get_palette_by_id(u8 index) {
  assert(index < 8);
  return OBP[index];
}