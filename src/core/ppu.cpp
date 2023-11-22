#include "core/ppu.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>

#include <cstddef>

#include "common.h"

PPU::PPU(){};

void PPU::set_renderer(SDL_Renderer* renderer) { this->renderer = renderer; }
void PPU::set_frame_texture(SDL_Texture* texture) {
  this->frame_texture = texture;
}
void PPU::add_sprite_to_buffer(u8 sprite_index) {
  // fmt::println("1");
  // fmt::println("{:#04x}", bus->oam.data[0]);
  u8 sprite_x     = bus->oam.data[sprite_index];
  u8 sprite_y     = bus->oam.data[sprite_index + 1];
  u8 tile_number  = bus->oam.data[sprite_index + 2];
  u8 sprite_flags = bus->oam.data[sprite_index + 3];

  if (!(sprite_x > 0)) {
    return;
  }
  // fmt::println("2");
  if (!(bus->wram.data[LY] + 16 > sprite_y)) {
    return;
  }

  if (!(bus->wram.data[LY] + 16 < sprite_y + (lcdc.obj_size ? 16 : 0))) {
    return;
  }

  if (!(sprite_count < 10)) {
    return;
  }

  Sprite s{sprite_y, sprite_x, tile_number, sprite_flags};
  sprite_buf.at(sprite_count++) = s;
}
void PPU::tick() {
  assert(bus != NULL);

  dots++;
  if (bus->wram.data[LY] >= 144 && bus->wram.data[LY] <= 153) {
    set_ppu_mode(RENDERING_MODE::VBLANK);

    if (dots == 456 && bus->wram.data[LY] == 153) {
      dots               = 0;
      bus->wram.data[LY] = 0;
      frame_queued       = true;
      set_ppu_mode(RENDERING_MODE::OAM_SCAN);
      return;
    }
  } else {
    if (dots <= 80) {
      if ((dots % 2) == 0) {
        // fmt::println("OAM INDEX: {:d}",oam_index);
        add_sprite_to_buffer(oam_index++);
      }
      return set_ppu_mode(RENDERING_MODE::OAM_SCAN);
    }
    if (dots > 80 && dots <= 252) {
      return set_ppu_mode(RENDERING_MODE::PIXEL_DRAW);
    }
    if (dots >= 253 && dots < 456) {
      return set_ppu_mode(RENDERING_MODE::HBLANK);
    }
  }

  if (dots == 456) {
    dots = 0;

    // reset oam buffer sprite count, fetch new ones next scanline
    sprite_count = 0;
    oam_index = 0;
    bus->wram.data[LY]++;
  }
  if ((bus->wram.data[LY] == bus->wram.data[LYC]) &&
      bus->wram.data[STAT] & 1 << 6) {
    return bus->request_interrupt(InterruptType::LCD);
  }
}
void PPU::render_tile_maps_to_texture() {
  SDL_SetRenderTarget(renderer, tile_map_0);
  for (u16 address = 0x8000; address < 0x9FFF; address++) {
    u8 index = bus->vram.read8(address - VRAM_ADDRESS_OFFSET);
    if (index == 0) {
      continue;
    }
    SDL_Log("%02X", index);
  }
}
void PPU::render_frame() {
  SDL_SetRenderTarget(renderer, frame_texture);

  assert(frame_texture != NULL);
}
u8 PPU::get_ppu_mode() { return bus->wram.data[STAT] & 0x3; };
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
    default: {
      return "INVALID MODE";
    }
  }
};
void PPU::set_ppu_mode(RENDERING_MODE _mode) {
  if (!(this->ppu_mode == _mode)) {
    ppu_mode = _mode;
    bus->wram.data[STAT] &= 0xfc;
    bus->wram.data[STAT] |= ppu_mode;

    if ((bus->wram.data[STAT] & 1 << ppu_mode) != 0 && ppu_mode != 3) {
      bus->request_interrupt(InterruptType::LCD);
    }
    if (ppu_mode == RENDERING_MODE::VBLANK) {
      bus->request_interrupt(InterruptType::VBLANK);
      frame_queued = true;
    }
  }
};