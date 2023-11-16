#include "core/ppu.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>

#include <cstddef>

PPU::PPU(){};

void PPU::render_frame() {
  SDL_SetRenderTarget(renderer, frame_texture);
  
  if (frame_texture == NULL) {
    SDL_Log("no frame texture!");
    exit(-1);
  }
  
  srand(iterator++);
  SDL_SetRenderDrawColor(renderer, rand(), rand(), rand(), 255);
  SDL_RenderDrawLine(renderer, iterator, 0 + iterator, 50 + iterator,
                     100 + iterator);

  srand(iterator++);
  SDL_SetRenderDrawColor(renderer, rand(), rand(), rand(), 255);
  SDL_RenderDrawLine(renderer, iterator, 0 + iterator, 50 + iterator,
                     100 + iterator);

                     

  if (iterator >= 0xFF) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
  }
}
