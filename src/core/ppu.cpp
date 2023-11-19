#include "core/ppu.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>

PPU::PPU(){};

void PPU::render_frame() {
  SDL_SetRenderTarget(renderer, frame_texture);

  assert(frame_texture != NULL);

}
