#include "frontend/window.h"

#include "common.h"
#include "cpu.h"
#include "imgui.h"
#include "io.hpp"
#include "tinyfiledialogs.h"
void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) {
      state.running = false;
    }
  }
}

void Frontend::show_menubar() {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::Separator();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load ROM")) {
        // draw_gui();
        auto path = tinyfd_openFileDialog("Load ROM", ".", 2, config.patterns,
                                          "Gameboy ROMs", 0);
        if (path != NULL) {
          this->gb->load_cart(read_file(path));
        }
      }
      if (ImGui::MenuItem("Reset")) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("State")) {
      if (ImGui::MenuItem("Load state..")) {}
      if (ImGui::MenuItem("Reset")) {}
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
void Frontend::show_ppu_info() {
  ImGui::Begin("PPU INFO", &state.cpu_info_open, 0);
  ImGui::Text("pointer to ppu = %p", (void*)&gb->ppu);
  ImGui::Text("PPU MODE = %s", gb->ppu.get_mode_string().c_str());
  ImGui::Text("dots = %d", gb->ppu.dots);
  ImGui::Text("LY (scanline) = %d", gb->bus.wram.data[LY]);
  ImGui::Text("LYC = %d", gb->bus.wram.data[LYC]);
  ImGui::Text("scanline sprite count = %d", gb->ppu.sprite_count);
  ImGui::Text("oam index = %d", gb->ppu.sprite_index);
  ImGui::Separator();
  ImGui::Text("x pos offset = %d", gb->ppu.x_pos_offset);
  ImGui::Separator();
  ImGui::Text("SCX = %d", gb->bus.wram.data[SCX]);
  ImGui::Text("SCY = %d", gb->bus.wram.data[SCY]);
  ImGui::Text("WY = %d", gb->bus.wram.data[WY]);
  ImGui::Text("WX = %d", gb->bus.wram.data[WX]);
  ImGui::Text("STAT = %d", gb->bus.wram.data[STAT]);
  ImGui::Text("%s",
              fmt::format("STAT = {:08b}", gb->bus.wram.data[STAT]).c_str());

  ImGui::Separator();
  ImGui::Text("%s",
              fmt::format("LCDC =  {:08b}", gb->bus.wram.data[LCDC]).c_str());
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

  // if (ImGui::Button("Print PPU info")) {
  //   gb->ppu.lcdc.print_status();
  // }

  // gb->ppu.lcdc.print_status();
  if (ImGui::Button("Trigger VBLANK interrupt")) {
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
  ImGui::Text("STATUS: %s", gb->cpu.get_cpu_mode().c_str());
  ImGui::Separator();
  ImGui::Text("Z: %x", gb->cpu.get_flag(Umibozu::FLAG::ZERO));
  ImGui::Text("N: %x", gb->cpu.get_flag(Umibozu::FLAG::NEGATIVE));
  ImGui::Text("H: %x", gb->cpu.get_flag(Umibozu::FLAG::HALF_CARRY));
  ImGui::Text("C: %x", gb->cpu.get_flag(Umibozu::FLAG::CARRY));
  ImGui::Separator();
  ImGui::Text("pointer to cpu = %p", (void*)&gb->cpu);
  ImGui::Text("PC = 0x%x", gb->cpu.PC);
  ImGui::Text("OPCODE: 0x%x", gb->cpu.peek(gb->cpu.PC));
  ImGui::Text("STATUS: %s", gb->cpu.get_cpu_mode().c_str());
  ImGui::Separator();
  ImGui::Text("DIV = 0x%x", gb->cpu.timer.get_div());
  ImGui::Text("timer enabled = %d", gb->cpu.timer.ticking_enabled);
  ImGui::Text("TIMA = 0x%x", gb->cpu.timer.counter);

  ImGui::Text("TMA = 0x%x", gb->cpu.timer.modulo);

  ImGui::Text("%s",
              fmt::format("TAC:  {:08b}", gb->bus.wram.data[TAC]).c_str());

  ImGui::Separator();
  ImGui::Text("%s", fmt::format("IME: {}", gb->cpu.IME).c_str());
  ImGui::Text("%s", fmt::format("IF:  {:08b}", gb->bus.wram.data[IF]).c_str());
  ImGui::Text("%s", fmt::format("IE:  {:08b}", gb->bus.wram.data[IE]).c_str());
  // ImGui::Text("%s", fmt::format("1:   {:08b}", 1).c_str());

  if (ImGui::Button("STEP")) {
    gb->cpu.run_instruction();
  }
  if (ImGui::Button("START")) {
    gb->cpu.status = CPU_STATUS::ACTIVE;
  }
  if (ImGui::Button("PAUSE")) {
    gb->cpu.status = CPU_STATUS::PAUSED;
  }

  ImGui::End();
}
void Frontend::show_tile_maps() {
  ImGui::Begin("Tile Maps", &state.texture_window_open, 0);
  ImGui::Image((void*)state.tile_map_texture_0, ImVec2(256, 256));
  ImGui::Separator();
  ImGui::Image((void*)state.tile_map_texture_1, ImVec2(256, 256));
  ImGui::End();
}
void Frontend::render_frame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  // ImGui::ShowDemoWindow(&state.demo_window_open);
  show_menubar();
  show_cpu_info();
  show_ppu_info();
  show_viewport();
  show_tile_maps();

  // ImGui::ShowDebugLogWindow(&state.debug_log_window_open);

  // Rendering
  ImGui::Render();
  SDL_RenderSetScale(renderer, state.io->DisplayFramebufferScale.x,
                     state.io->DisplayFramebufferScale.y);
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  SDL_SetRenderTarget(renderer, NULL);

  SDL_SetRenderDrawColor(renderer, (u8)(clear_color.x * 255),
                         (u8)(clear_color.y * 255), (u8)(clear_color.z * 255),
                         (u8)(clear_color.w * 255));
  SDL_RenderClear(renderer);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
  SDL_RenderPresent(renderer);
}
void Frontend::show_viewport() {
  ImGui::Begin("viewport", &state.texture_window_open, 0);
  ImGui::Text("pointer to texture = %p", (void*)&state.ppu_texture);
  ImGui::Text("pointer to gb instance = %p", (void*)&gb);
  ImGui::Text("fps = %f", state.io->Framerate);

  ImGui::Image((void*)state.ppu_texture, ImVec2(160 * 2, 144 * 2));

  ImGui::End();
}
Frontend::Frontend() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fmt::println("ERROR: failed initialize SDL");
    exit(-1);
  };

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window =
      SDL_CreateWindow("umibozu", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 256);

  this->state.tile_map_texture_0 = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 256);

  this->state.tile_map_texture_1 = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 256);

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}