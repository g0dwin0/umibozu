#include "frontend/window.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_video.h>

#include <tuple>

#include "bus.hpp"
#include "common.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "io.hpp"
#include "joypad.hpp"
#include "tinyfiledialogs.h"
static MemoryEditor editor_instance;

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      case SDL_QUIT: {
        state.running = false;
        gb->active    = false;
        break;
      }

      case SDL_KEYDOWN: {
        if (state.keyboardState[settings.keybinds.control_map.at(RIGHT).first]) {
          gb->bus.joypad.RIGHT = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(LEFT).first]) {
          gb->bus.joypad.LEFT = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(UP).first]) {
          gb->bus.joypad.UP = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(DOWN).first]) {
          gb->bus.joypad.DOWN = 0;
        }

        if (state.keyboardState[settings.keybinds.control_map.at(A).first]) {
          gb->bus.joypad.A = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(B).first]) {
          gb->bus.joypad.B = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(START).first]) {
          gb->bus.joypad.START = 0;
        }
        if (state.keyboardState[settings.keybinds.control_map.at(SELECT).first]) {
          gb->bus.joypad.SELECT = 0;
        }

        break;
      }

      case SDL_KEYUP: {
        // fmt::println("Triggered UP");
        if (!state.keyboardState[settings.keybinds.control_map.at(RIGHT).first]) {
          gb->bus.joypad.RIGHT = 1;
        }
        if (!state.keyboardState[settings.keybinds.control_map.at(LEFT).first]) {
          gb->bus.joypad.LEFT = 1;
        }

        if (!state.keyboardState[settings.keybinds.control_map.at(UP).first]) {
          gb->bus.joypad.UP = 1;
        }
        if (!state.keyboardState[settings.keybinds.control_map.at(DOWN).first]) {
          gb->bus.joypad.DOWN = 1;
        }

        if (!state.keyboardState[settings.keybinds.control_map.at(A).first]) {
          gb->bus.joypad.A = 1;
        }
        if (!state.keyboardState[settings.keybinds.control_map.at(B).first]) {
          gb->bus.joypad.B = 1;
        }
        if (!state.keyboardState[settings.keybinds.control_map.at(START).first]) {
          gb->bus.joypad.START = 1;
        }
        if (!state.keyboardState[settings.keybinds.control_map.at(SELECT).first]) {
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
  ImGui::Text("LCDC = %02X", gb->bus.io[LCDC]);
  ImGui::Text("STAT = %02X", gb->bus.io[STAT]);
  ImGui::Text("SCY = %02X", gb->bus.io[SCY]);
  ImGui::Text("SCX = %02X", gb->bus.io[SCX]);
  ImGui::Text("LY = %02X", gb->bus.io[LY]);
  ImGui::Text("LYC = %02X", gb->bus.io[LYC]);
  ImGui::Text("DMA = %02X", gb->bus.io[DMA]);
  ImGui::Text("BGP = %02X", gb->bus.io[BGP]);
  ImGui::Text("OBP0 = %02X", gb->bus.io[OBP0]);
  ImGui::Text("OBP1 = %02X", gb->bus.io[OBP1]);
  ImGui::Text("WY = %02X", gb->bus.io[WY]);
  ImGui::Text("WX = %02X", gb->bus.io[WX]);

  ImGui::Text("SVBK = %02X", gb->bus.io[SVBK]);
  ImGui::Text("VBK = %02X", gb->bus.io[VBK]);
  ImGui::Text("KEY1 = %02X", gb->bus.io[KEY1]);
  ImGui::Text("JOYP = %02X", gb->bus.io[JOYPAD]);
  ImGui::Text("SB = %02X", gb->bus.io[SB]);
  ImGui::Text("SC = %02X", gb->bus.io[SC]);

  ImGui::Text("TIMA = %02X", gb->bus.io[TIMA]);
  ImGui::Text("TMA = %02X", gb->bus.io[TMA]);
  ImGui::Text("IF = %02X", gb->bus.io[IF]);
  ImGui::Text("IE = %02X", gb->bus.io[IE]);

  ImGui::End();
}

void Frontend::show_controls_menu(bool* p_open) {
  ImGui::Begin("Controls", p_open);
  ImGui::Text("Remap your controls here.");
  ImGui::Separator();

  bool invalid_keybind = false;

  for (SDL_Scancode start = SDL_SCANCODE_A; start < SDL_SCANCODE_AUDIOFASTFORWARD; start = (SDL_Scancode)(start + 1)) {
    if (!state.keyboardState[start]) continue;
    ImGui::SameLine();

    // refactor: hard to read; use structured bindings
    for (auto& entry : settings.keybinds.control_map) {
      // If button is set to be remapped, remap with next key input
      if (entry.second.second) {
        entry.second.second = false;
        for (auto& diff_entry : settings.keybinds.control_map) {
          if (diff_entry.first != entry.first && diff_entry.second.first == start) {
            invalid_keybind = true;
          }
        }
        if (!invalid_keybind) {
          entry.second.first  = start;
          entry.second.second = false;  // unset remap flag
        }
      }
    }
  }

  ImGui::Separator();
  ImGui::Spacing();

  ImGui::SameLine();

  for (auto& entry : settings.keybinds.control_map) {
    ImGui::PushID(&entry);

    ImGui::Text("%s: ", get_button_name_from_enum(entry.first).c_str());
    ImGui::SameLine();
    if (ImGui::Button(entry.second.second ? "Waiting for input..." : SDL_GetScancodeName(entry.second.first))) {
      entry.second.second = true;  // Being remapped
    };
    ImGui::PopID();
  }
  ImGui::End();
}
void Frontend::show_menubar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        auto path = tinyfd_openFileDialog("Load ROM", ".", 1, patterns, "Gameboy ROMs", 0);
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
    if (ImGui::BeginMenu("Settings")) {
      if (ImGui::MenuItem("Controls")) {
        this->state.controls_window_open = !this->state.controls_window_open;
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
  ImGui::Text("LY (scanline) = %d", gb->bus.io[LY]);
  ImGui::Text("LYC = %d", gb->bus.io[LYC]);
  ImGui::Text("WRAM BANK = %d", gb->bus.svbk);
  ImGui::Text("VRAM BANK = %d", gb->bus.vbk);
  if (gb->bus.mode == SYSTEM_MODE::CGB) {
    ImGui::Text("OPRI = %s", gb->bus.io[OPRI] == 1 ? "DMG-style" : "CGB-style");
  }
  ImGui::Separator();
  ImGui::Text("x pos offset = %d", gb->ppu.x_pos_offset);
  ImGui::Separator();
  ImGui::Text("SCX = %d", gb->bus.io[SCX]);
  ImGui::Text("SCY = %d", gb->bus.io[SCY]);
  ImGui::Text("WX = %d", gb->bus.io[WX]);
  ImGui::Text("WY = %d", gb->bus.io[WY]);
  ImGui::Text("STAT = %d", gb->bus.io[STAT]);
  ImGui::Text("%s", fmt::format("STAT = {:08b}", gb->bus.io[STAT]).c_str());

  ImGui::Separator();
  if (ImGui::Button("Framelimit to 60")) {
    gb->ppu.target_duration = std::chrono::duration<double, std::milli>(16.7);
  }
  if (ImGui::Button("Framelimit to 120")) {
    gb->ppu.target_duration = std::chrono::duration<double, std::milli>((16.7 / 2));

  }
  if (ImGui::Button("Uncap framerate")) {

  }
  
  ImGui::Separator();

  ImGui::Text("%s", fmt::format("LCDC =  {:08b}", gb->bus.io[LCDC]).c_str());
  ImGui::Text("LCDC.0 (BG & window enable) = %s", gb->ppu.lcdc.bg_and_window_enable_priority == 1 ? "True" : "False");
  ImGui::Text("LCDC.1 (OBJ enable) = %s", gb->ppu.lcdc.sprite_enable == 1 ? "On" : "Off");
  ImGui::Text("LCDC.2 (OBJ size) = %s", gb->ppu.lcdc.sprite_size == 1 ? "8x16" : "8x8");
  ImGui::Text("LCDC.3 (BG tile map area) = %s", gb->ppu.lcdc.bg_tile_map_select == 1 ? "0x9C00 - 0x9FFF" : "0x9800 - 0x9BFF");
  ImGui::Text("LCDC.4 (BG & Window tiles) = %s", gb->ppu.lcdc.tiles_select_method == 1 ? "0x8000" : "0x8800");
  ImGui::Text("LCDC.5 (Window enable) = %s", gb->ppu.lcdc.window_disp_enable == 1 ? "On" : "Off");
  ImGui::Text("LCDC.6 (Window tile map) = %s", gb->ppu.lcdc.window_tile_map_select == 1 ? "0x9C00 - 0x9FFF" : "0x9800 - 0x9BFF");
  ImGui::Text("LCDC.7 (LCD & PPU enable) = %s", gb->ppu.lcdc.lcd_ppu_enable == 1 ? "On" : "Off");

  if (ImGui::Button("Trigger VBLANK interrupt")) {
    fmt::println("VBLANK triggered");
    gb->bus.request_interrupt(INTERRUPT_TYPE::VBLANK);
  }
  if (ImGui::Button("Trigger LCD interrupt")) {
    gb->bus.request_interrupt(INTERRUPT_TYPE::LCD);
  }
  if (ImGui::Button("Trigger Timer interrupt")) {
    gb->bus.request_interrupt(INTERRUPT_TYPE::TIMER);
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
  ImGui::Text("SPEED: %s", gb->cpu.speed == Umibozu::SM83::SPEED::DOUBLE ? "DOUBLE" : "NORMAL");
  ImGui::Separator();

  ImGui::Text("Z: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::ZERO));
  ImGui::Text("N: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::NEGATIVE));
  ImGui::Text("H: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::HALF_CARRY));
  ImGui::Text("C: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::CARRY));
  ImGui::Separator();
  ImGui::Text("pointer to cpu = %p", (void*)&gb->cpu);
  ImGui::Text("pc = 0x%x", gb->cpu.PC);
  ImGui::Text("opcode: 0x%x", gb->cpu.peek(gb->cpu.PC));
  ImGui::Text("status: %s", gb->cpu.get_cpu_mode_string().c_str());
  ImGui::Separator();
  ImGui::Text("DIV = 0x%x", gb->cpu.timer->get_div());
  ImGui::Text("TIMA = 0x%x", gb->cpu.timer->counter);
  ImGui::Text("TMA = 0x%x", gb->cpu.timer->modulo);
  ImGui::Text("timer enabled = %d", gb->cpu.timer->ticking_enabled);
  ImGui::Separator();
  ImGui::Text("mode = %s", gb->bus.get_mode_string().c_str());
  ImGui::Separator();
  ImGui::Text("%s", fmt::format("JOYPAD:  {:08b}", gb->bus.io[JOYPAD]).c_str());
  ImGui::Separator();

  ImGui::Text("%s", fmt::format("TAC:  {:08b}", gb->bus.io[TAC]).c_str());
  ImGui::Text("%s", fmt::format("STAT:  {:08b}", gb->bus.io[STAT]).c_str());

  ImGui::Separator();
  ImGui::Text("%s", fmt::format("IME: {}", gb->cpu.IME).c_str());
  ImGui::Text("%s", fmt::format("IF:  {:08b}", gb->bus.io[IF]).c_str());
  ImGui::Text("%s", fmt::format("IE:  {:08b}", gb->bus.io[IE]).c_str());

  if (ImGui::Button("STEP")) {
    gb->cpu.run_instruction();
  }
  if (ImGui::Button("START")) {
    gb->cpu.status = SM83::STATUS::ACTIVE;
  }
  if (ImGui::Button("PAUSE")) {
    gb->cpu.status = SM83::STATUS::PAUSED;
  }

  ImGui::End();
}

void Frontend::render_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  // show_menubar();
  if (state.controls_window_open) {
    show_controls_menu(&state.controls_window_open);
  }

  // #ifdef DEBUG_MODE_H
  show_viewport();
  show_cpu_info();
  show_ppu_info();
  show_memory_viewer();

  // show_tile_maps();
  show_io_info();
  // #endif
  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x, state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  SDL_UpdateTexture(state.ppu_texture, nullptr, gb->ppu.db.disp_buf, 256 * 2);
  SDL_RenderCopy(renderer, state.ppu_texture, NULL, NULL);

  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}
void Frontend::show_viewport() {
  ImGui::Begin("viewport", &state.texture_window_open, 0);
  ImGui::Text("pointer to texture = %p", (void*)&state.ppu_texture);
  ImGui::Text("pointer to gb instance = %p", (void*)&gb);
  ImGui::Text("fps = %f", state.io->Framerate);

  ImGui::Image((void*)state.ppu_texture, ImVec2(256 * 2, 256 * 2));

  ImGui::End();
}
Frontend::Frontend(GB* gb) {
  this->gb = gb;
  assert(this->gb != nullptr);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
    fmt::println("ERROR: failed initialize SDL");
    exit(-1);
  }

  auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  this->window   = SDL_CreateWindow("umibozu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  // io.ConfigFlags |=
  //     ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  this->state.io = &io;

  ImGui::StyleColorsDark();

  this->state.ppu_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 256, 256);

  this->state.viewport = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_TARGET, 256, 256);

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}

void Frontend::show_memory_viewer() {
  ImGui::Begin("Memory Viewer", &state.memory_viewer_open, 0);
  const char* regions[] = {
      "Region: Frame",
  };

  const void* memory_partitions[] = {&gb->ppu.frame.data

  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) {
    // SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]);
  }
  editor_instance.OptShowAscii = false;
  // editor_instance.ReadOnly = true;

  editor_instance.DrawContents((void*)memory_partitions[SelectedItem], 256 * 256);

  ImGui::End();
}