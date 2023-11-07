#include "frontend/window.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_render.h>

#include <cstddef>

#include "imgui.h"

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) {
      state.running = false;
    }
  }
}

void Frontend::render_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  show_viewport();

  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  SDL_SetRenderTarget(renderer, this->state.ppu_texture);
  SDL_RenderDrawLine(renderer, 10, 10, 500, 500);

  SDL_SetRenderTarget(renderer, NULL);
  
  SDL_SetRenderDrawColor(
      renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255),
      (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
  SDL_RenderClear(renderer);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}
void Frontend::show_viewport() {
  ImGui::Begin("viewport", &state.texture_window_open, 0);
  ImGui::Text("pointer = %p", (void*)&state.ppu_texture);
  ImGui::Image((void*)state.ppu_texture, ImVec2(160, 144));

  ImGui::End();
  ImGui::Render();
}

Frontend::Frontend() {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fmt::println("ERROR: failed initialize SDL");
    exit(-1);
  };

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window =
      SDL_CreateWindow("umibozu", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  this->window   = window;
  this->renderer = renderer;

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  this->state.io = &io;
  this->window   = window;

  ImGui::StyleColorsDark();

  // setup output graphical output texture
  this->state.ppu_texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 160, 144);

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}