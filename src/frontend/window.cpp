#include "frontend/window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>


#include "bus.h"
#include "common.h"
#include "cpu.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "io.hpp"
#include "tinyfiledialogs.h"

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      case SDL_QUIT: {
        state.running = false;
        break;
      }

      case SDL_KEYDOWN: {
        const u8* keyboardState = SDL_GetKeyboardState(nullptr);
        if (keyboardState[SDL_SCANCODE_RIGHT]) {
          gb->bus.joypad.RIGHT = 0;
        }
        if (keyboardState[SDL_SCANCODE_LEFT]) {
          gb->bus.joypad.LEFT = 0;
        }
        if (keyboardState[SDL_SCANCODE_UP]) {
          gb->bus.joypad.UP = 0;
        }
        if (keyboardState[SDL_SCANCODE_DOWN]) {
          gb->bus.joypad.DOWN = 0;
        }

        if (keyboardState[SDL_SCANCODE_Z]) {
          gb->bus.joypad.A = 0;
        }
        if (keyboardState[SDL_SCANCODE_X]) {
          gb->bus.joypad.B = 0;
        }
        if (keyboardState[SDL_SCANCODE_RETURN]) {
          gb->bus.joypad.START = 0;
        }
        if (keyboardState[SDL_SCANCODE_BACKSPACE]) {
          gb->bus.joypad.SELECT = 0;
        }

        break;
      }

      case SDL_KEYUP: {
        const u8* keyboardState = SDL_GetKeyboardState(nullptr);

        if (!keyboardState[SDL_SCANCODE_RIGHT]) {
          gb->bus.joypad.RIGHT = 1;
        }
        if (!keyboardState[SDL_SCANCODE_LEFT]) {
          gb->bus.joypad.LEFT = 1;
        }
        if (!keyboardState[SDL_SCANCODE_UP]) {
          gb->bus.joypad.UP = 1;
        }
        if (!keyboardState[SDL_SCANCODE_DOWN]) {
          gb->bus.joypad.DOWN = 1;
        }

        if (!keyboardState[SDL_SCANCODE_Z]) {
          gb->bus.joypad.A = 1;
        }
        if (!keyboardState[SDL_SCANCODE_X]) {
          gb->bus.joypad.B = 1;
        }
        if (!keyboardState[SDL_SCANCODE_RETURN]) {
          gb->bus.joypad.START = 1;
        }
        if (!keyboardState[SDL_SCANCODE_BACKSPACE]) {
          gb->bus.joypad.SELECT = 1;
        }

        break;
      }
    }
  }
}

void Frontend::shutdown() {
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
}
void Frontend::show_io_info() {
  ImGui::Begin("IO INFO", &state.ppu_info_open, 0);
  ImGui::Text("LCDC = %02X", gb->bus.io.data[LCDC]);
  ImGui::Text("STAT = %02X", gb->bus.io.data[STAT]);
  ImGui::Text("SCY = %02X", gb->bus.io.data[SCY]);
  ImGui::Text("SCX = %02X", gb->bus.io.data[SCX]);
  ImGui::Text("LY = %02X", gb->bus.io.data[LY]);
  ImGui::Text("LYC = %02X", gb->bus.io.data[LYC]);
  ImGui::Text("DMA = %02X", gb->bus.io.data[DMA]);
  ImGui::Text("BGP = %02X", gb->bus.io.data[BGP]);
  ImGui::Text("OBP0 = %02X", gb->bus.io.data[OBP0]);
  ImGui::Text("OBP1 = %02X", gb->bus.io.data[OBP1]);
  ImGui::Text("WY = %02X", gb->bus.io.data[WY]);
  ImGui::Text("WX = %02X", gb->bus.io.data[WX]);

  ImGui::Text("SVBK = %02X", gb->bus.io.data[SVBK]);
  ImGui::Text("VBK = %02X", gb->bus.io.data[VBK]);
  ImGui::Text("KEY1 = %02X", gb->bus.io.data[KEY1]);
  ImGui::Text("JOYP = %02X", gb->bus.io.data[JOYPAD]);
  ImGui::Text("SB = %02X", gb->bus.io.data[SB]);
  ImGui::Text("SC = %02X", gb->bus.io.data[SC]);

  ImGui::Text("TIMA = %02X", gb->bus.io.data[TIMA]);
  ImGui::Text("TMA = %02X", gb->bus.io.data[TMA]);
  ImGui::Text("IF = %02X", gb->bus.io.data[IF]);
  ImGui::Text("IE = %02X", gb->bus.io.data[IE]);

  ImGui::End();
}
void Frontend::show_menubar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        auto path = tinyfd_openFileDialog("Load ROM", ".", 1, patterns,
                                          "Gameboy ROMs", 0);
        if (path != nullptr) {
          this->gb->load_cart(read_file(path));
        }
      }

      if (ImGui::MenuItem("Reset")) {
        if (!gb->bus.cart.info.path.empty()) {
          this->gb->load_cart(read_file(gb->bus.cart.info.path));
        }
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}
void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.ppu_info_open, 0);
  ImGui::Text("pointer to ppu = %p", (void*)&gb->ppu);
  ImGui::Text("PPU MODE = %s", gb->ppu.get_mode_string().c_str());
  ImGui::Text("dots = %d", gb->ppu.dots);
  ImGui::Text("LY (scanline) = %d", gb->bus.io.data[LY]);
  ImGui::Text("LYC = %d", gb->bus.io.data[LYC]);
  ImGui::Text("WRAM BANK = %d", gb->bus.svbk);
  ImGui::Text("VRAM BANK = %d", gb->bus.vbk);
  if (gb->bus.mode == COMPAT_MODE::CGB_ONLY) {
    ImGui::Text("OPRI = %s",
                gb->bus.io.data[OPRI] == 1 ? "DMG-style" : "CGB-style");
  }
  ImGui::Separator();
  ImGui::Text("x pos offset = %d", gb->ppu.x_pos_offset);
  ImGui::Separator();
  ImGui::Text("SCX = %d", gb->bus.io.data[SCX]);
  ImGui::Text("SCY = %d", gb->bus.io.data[SCY]);
  ImGui::Text("WX = %d", gb->bus.io.data[WX]);
  ImGui::Text("WY = %d", gb->bus.io.data[WY]);
  ImGui::Text("STAT = %d", gb->bus.io.data[STAT]);
  ImGui::Text("%s",
              fmt::format("STAT = {:08b}", gb->bus.io.data[STAT]).c_str());

  ImGui::Separator();
  if (ImGui::Button("Enable VSYNC")) {
    SDL_GL_SetSwapInterval(1);
  }
  if (ImGui::Button("Disable VSYNC")) {
    SDL_GL_SetSwapInterval(0);
  }
  ImGui::Separator();

  ImGui::Text("%s",
              fmt::format("LCDC =  {:08b}", gb->bus.io.data[LCDC]).c_str());
  ImGui::Text(
      "LCDC.0 (BG & window enable) = %s",
      gb->ppu.lcdc.bg_and_window_enable_priority == 1 ? "True" : "False");
  ImGui::Text("LCDC.1 (OBJ enable) = %s",
              gb->ppu.lcdc.sprite_enable == 1 ? "On" : "Off");
  ImGui::Text("LCDC.2 (OBJ size) = %s",
              gb->ppu.lcdc.sprite_size == 1 ? "8x16" : "8x8");
  ImGui::Text("LCDC.3 (BG tile map area) = %s",
              gb->ppu.lcdc.bg_tile_map_select == 1 ? "0x9C00 - 0x9FFF"
                                                   : "0x9800 - 0x9BFF");
  ImGui::Text("LCDC.4 (BG & Window tiles) = %s",
              gb->ppu.lcdc.tiles_select_method == 1 ? "0x8000" : "0x8800");
  ImGui::Text("LCDC.5 (Window enable) = %s",
              gb->ppu.lcdc.window_disp_enable == 1 ? "On" : "Off");
  ImGui::Text("LCDC.6 (Window tile map) = %s",
              gb->ppu.lcdc.window_tile_map_select == 1 ? "0x9C00 - 0x9FFF"
                                                       : "0x9800 - 0x9BFF");
  ImGui::Text("LCDC.7 (LCD & PPU enable) = %s",
              gb->ppu.lcdc.lcd_ppu_enable == 1 ? "On" : "Off");

  if (ImGui::Button("Trigger VBLANK interrupt")) {
    fmt::println("VBLANK triggered");
    gb->bus.request_interrupt(InterruptType::VBLANK);
  }
  if (ImGui::Button("Trigger LCD interrupt")) {
    gb->bus.request_interrupt(InterruptType::LCD);
  }
  if (ImGui::Button("Trigger Timer interrupt")) {
    gb->bus.request_interrupt(InterruptType::TIMER);
  }

  if (ImGui::Button("STEP")) {
    gb->cpu.run_instruction();
  }
  ImGui::End();
}

void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  ImGui::Text("ROM BANK: %d", gb->cpu.mapper->rom_bank);
  ImGui::Text("RAM BANK: %d", gb->cpu.mapper->ram_bank);
  ImGui::Separator();
  ImGui::Text("Speed: %s", gb->cpu.speed == Umibozu::SM83::Speed::DOUBLE
                               ? "DOUBLE"
                               : "NORMAL");
  ImGui::Separator();

  ImGui::Text("Z: %x", gb->cpu.get_flag(Umibozu::SM83::Flag::ZERO));
  ImGui::Text("N: %x", gb->cpu.get_flag(Umibozu::SM83::Flag::NEGATIVE));
  ImGui::Text("H: %x", gb->cpu.get_flag(Umibozu::SM83::Flag::HALF_CARRY));
  ImGui::Text("C: %x", gb->cpu.get_flag(Umibozu::SM83::Flag::CARRY));
  ImGui::Separator();
  ImGui::Text("pointer to cpu = %p", (void*)&gb->cpu);
  ImGui::Text("pc = 0x%x", gb->cpu.PC);
  ImGui::Text("opcode: 0x%x", gb->cpu.peek(gb->cpu.PC));
  ImGui::Text("status: %s", gb->cpu.get_cpu_mode().c_str());
  ImGui::Separator();
  ImGui::Text("DIV = 0x%x", gb->cpu.timer.get_div());
  ImGui::Text("TIMA = 0x%x", gb->cpu.timer.counter);
  ImGui::Text("TMA = 0x%x", gb->cpu.timer.modulo);
  ImGui::Text("timer enabled = %d", gb->cpu.timer.ticking_enabled);
  ImGui::Separator();
  ImGui::Text("mode = %s", gb->bus.get_mode_string().c_str());
  ImGui::Separator();
  ImGui::Text("%s",
              fmt::format("JOYPAD:  {:08b}", gb->bus.io.data[JOYPAD]).c_str());
  ImGui::Separator();

  ImGui::Text("%s", fmt::format("TAC:  {:08b}", gb->bus.io.data[TAC]).c_str());

  ImGui::Separator();
  ImGui::Text("%s", fmt::format("IME: {}", gb->cpu.IME).c_str());
  ImGui::Text("%s", fmt::format("IF:  {:08b}", gb->bus.io.data[IF]).c_str());
  ImGui::Text("%s", fmt::format("IE:  {:08b}", gb->bus.io.data[IE]).c_str());

  if (ImGui::Button("STEP")) {
    gb->cpu.run_instruction();
  }
  if (ImGui::Button("START")) {
    gb->cpu.status = SM83::Status::ACTIVE;
  }
  if (ImGui::Button("PAUSE")) {
    gb->cpu.status = SM83::Status::PAUSED;
  }

  ImGui::End();
}

void Frontend::render_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  show_menubar();
#ifdef DEBUG_MODE_H
  show_viewport();
  show_cpu_info();
  show_ppu_info();
  show_tile_maps();
  show_io_info();
#endif
  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);
  SDL_Rect rect{0, 0, 160, 151};
  SDL_RenderCopy(renderer, state.ppu_texture, &rect, NULL);

  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
  SDL_Delay(16);
}
void Frontend::show_viewport() {
  ImGui::Begin("viewport", &state.texture_window_open, 0);
  ImGui::Text("pointer to texture = %p", (void*)&state.ppu_texture);
  ImGui::Text("pointer to gb instance = %p", (void*)&gb);
  ImGui::Text("fps = %f", state.io->Framerate);

  ImGui::Image((void*)state.ppu_texture, ImVec2(256 * 2, 256 * 2));

  ImGui::End();
}
Frontend::Frontend(GB& gb) {
  this->gb = &gb;
  assert(this->gb != nullptr);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) !=
      0) {
    fmt::println("ERROR: failed initialize SDL");
    exit(-1);
  }

  auto window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);

  this->window =
      SDL_CreateWindow("umibozu", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 160*3, 144*3, window_flags);
  this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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

  ImGui::StyleColorsDark();

  this->state.ppu_texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 256, 256);
  
  this->state.viewport = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 256, 256);
  
  

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}