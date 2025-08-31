#include "frontend/window.hpp"

#include <SDL3/SDL.h>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "bus.hpp"
#include "common.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include "io.hpp"
#include "io_defs.hpp"
#include "joypad.hpp"
#include "lib/tinyfiledialogs/tinyfiledialogs.h"
#include <filesystem>

static MemoryEditor editor_instance;

void Frontend::handle_events() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
      case SDL_EVENT_QUIT: {
        state.running = false;
        gb->active    = false;
        break;
      }

      case SDL_EVENT_KEY_DOWN: {
        if (state.keyboard_state[settings.keybinds.control_map.at(RIGHT).first]) {
          gb->bus.joypad.RIGHT = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(LEFT).first]) {
          gb->bus.joypad.LEFT = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(UP).first]) {
          gb->bus.joypad.UP = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(DOWN).first]) {
          gb->bus.joypad.DOWN = 0;
        }

        if (state.keyboard_state[settings.keybinds.control_map.at(A).first]) {
          gb->bus.joypad.A = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(B).first]) {
          gb->bus.joypad.B = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(START).first]) {
          gb->bus.joypad.START = 0;
        }
        if (state.keyboard_state[settings.keybinds.control_map.at(SELECT).first]) {
          gb->bus.joypad.SELECT = 0;
        }

        break;
      }

      case SDL_EVENT_KEY_UP: {
        if (!state.keyboard_state[settings.keybinds.control_map.at(RIGHT).first]) {
          gb->bus.joypad.RIGHT = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(LEFT).first]) {
          gb->bus.joypad.LEFT = 1;
        }

        if (!state.keyboard_state[settings.keybinds.control_map.at(UP).first]) {
          gb->bus.joypad.UP = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(DOWN).first]) {
          gb->bus.joypad.DOWN = 1;
        }

        if (!state.keyboard_state[settings.keybinds.control_map.at(A).first]) {
          gb->bus.joypad.A = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(B).first]) {
          gb->bus.joypad.B = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(START).first]) {
          gb->bus.joypad.START = 1;
        }
        if (!state.keyboard_state[settings.keybinds.control_map.at(SELECT).first]) {
          gb->bus.joypad.SELECT = 1;
        }

        break;
      }
    }
  }
}

void Frontend::shutdown() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_Quit();
}
void Frontend::show_io_info() {
  ImGui::Begin("IO INFO", &state.ppu_info_open, 0);
  ImGui::Text("(LCD Controller) LCDC = %#02x", gb->bus.io[LCDC]);
  ImGui::Text("STAT = %#02x", gb->bus.io[STAT]);
  ImGui::Text("(Scroll Y) SCY = %#02x", gb->bus.io[SCY]);
  ImGui::Text("(Scroll X) SCX = %#02x", gb->bus.io[SCX]);
  ImGui::Text("LY = %#02x", gb->bus.io[LY]);
  ImGui::Text("LYC = %#02x", gb->bus.io[LYC]);
  ImGui::Text("DMA = %#02x", gb->bus.io[DMA]);
  ImGui::Text("(Background Palette) BGP = %#02x", gb->bus.io[BGP]);
  ImGui::Text("OBP0 = %#02x", gb->bus.io[OBP0]);
  ImGui::Text("OBP1 = %#02x", gb->bus.io[OBP1]);
  ImGui::Text("(Window Y) WY = %#02x", gb->bus.io[WY]);
  ImGui::Text("(Window X) WX = %#02x", gb->bus.io[WX]);

  ImGui::Text("(WRAM BANK) SVBK = %#02x", gb->bus.io[SVBK]);
  ImGui::Text("(VRAM BANK) VBK = %#02x", gb->bus.io[VBK]);
  ImGui::Text("KEY1 = %#02x", gb->bus.io[KEY1]);
  ImGui::Text("JOYP = %#02x", gb->bus.io[JOYPAD]);
  ImGui::Text("SB = %#02x", gb->bus.io[SB]);
  ImGui::Text("SC = %#02x", gb->bus.io[SC]);

  ImGui::Text("TIMA = %#02x", gb->bus.io[TIMA]);
  ImGui::Text("TMA = %#02x", gb->bus.io[TMA]);
  ImGui::Text("IF = %#02x", gb->bus.io[IF]);
  ImGui::Text("IE = %#02x", gb->bus.io[IE]);

  ImGui::End();
}

void Frontend::show_controls_menu(bool* p_open) {
  ImGui::Begin("Controls", p_open);
  ImGui::Text("Remap your controls here.");
  ImGui::Separator();

  bool invalid_keybind = false;

  for (SDL_Scancode start = SDL_SCANCODE_A; start < SDL_SCANCODE_MEDIA_FAST_FORWARD; start = (SDL_Scancode)(start + 1)) {
    if (!state.keyboard_state[start]) continue;
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
        auto path = tinyfd_openFileDialog("Load ROM", "roms/", 2, patterns, "Gameboy ROM", 0);
        if (path != nullptr) {
          gb->reset();
          this->gb->load_cart(read_file(path));
        }
      }

      if (ImGui::MenuItem("Reset")) {
        if (!gb->cart.info.path.empty()) {
          gb->reset();
          this->gb->load_cart(read_file(gb->cart.info.path));
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

    if (ImGui::BeginMenu("Debug Options")) {
      ImGui::Checkbox("Memory Viewer", &state.memory_viewer_open);
      ImGui::Checkbox("CPU Info", &state.cpu_info_open);
      ImGui::Checkbox("PPU Info", &state.ppu_info_open);
      ImGui::Checkbox("IO Info", &state.io_info_open);
      ImGui::Checkbox("APU Info", &state.apu_info_open);

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
void Frontend::show_apu_info() {
  ImGui::Begin("APU Info", &state.apu_info_open, 0);

  bool a = gb->apu.regs.NR52.AUDIO_ON;
  bool b = gb->apu.regs.channel_1.dac_enabled();
  bool c = gb->apu.regs.channel_2.dac_enabled();

  ImGui::Checkbox("APU [NR52] - Enabled {}", &a);

  ImGui::SeparatorText("Channel 1 - Square + Sweep");
  ImGui::Checkbox("Enabled##x", &gb->apu.regs.channel_1.channel_enabled);
  ImGui::Checkbox("DAC Enabled##xxx", &b);
  ImGui::Text("Length Timer: %d", gb->apu.regs.channel_1.length_timer);
  ImGui::Text("Duty Step: %d", gb->apu.regs.channel_1.duty_step);

  ImGui::SeparatorText("Channel 2 - Square");
  ImGui::Checkbox("Enabled##xx", &gb->apu.regs.channel_2.channel_enabled);
  ImGui::Checkbox("DAC Enabled##x", &c);
  ImGui::Text("Length Timer: %d", gb->apu.regs.channel_2.length_timer);
  ImGui::Text("Duty Step: %d", gb->apu.regs.channel_2.duty_step);

  ImGui::SeparatorText("Channel 3 - Wave");
  ImGui::Checkbox("Enabled##xxx", &gb->apu.regs.channel_3.channel_enabled);

  ImGui::SeparatorText("Channel 4 - Noise");
  ImGui::Checkbox("Enabled##xxxx", &gb->apu.regs.channel_4.channel_enabled);

  ImGui::SeparatorText("Sample Buffer");
  ImGui::Text("Write position: %d", gb->apu.write_pos);
  ImGui::Text("Sample value: %.3f", gb->apu.audio_buf[gb->apu.write_pos]);

  ImGui::SeparatorText("Sound Panning");
  ImGui::Text("NR51: %d", gb->apu.regs.NR51.value);
  ImGui::Text("CH1 L: %d R: %d", +gb->apu.regs.NR51.CH1_LEFT, +gb->apu.regs.NR51.CH1_RIGHT);
  ImGui::Text("CH2 L: %d R: %d", +gb->apu.regs.NR51.CH2_LEFT, +gb->apu.regs.NR51.CH2_RIGHT);
  ImGui::Text("CH3 L: %d R: %d", +gb->apu.regs.NR51.CH3_LEFT, +gb->apu.regs.NR51.CH3_RIGHT);
  ImGui::Text("CH4 L: %d R: %d", +gb->apu.regs.NR51.CH4_LEFT, +gb->apu.regs.NR51.CH4_RIGHT);
  ImGui::End();
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
  if (ImGui::Button("Uncap framerate")) {}

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

  if (ImGui::Button("Toggle Window")) {
    gb->ppu.lcdc.window_disp_enable ^= 1;
  }

  if (ImGui::Button("STEP")) {
    gb->cpu.run_instruction();
  }
  ImGui::End();
}

void Frontend::show_cpu_info() {
  ImGui::Begin("CPU INFO", &state.cpu_info_open, 0);
  if (gb->bus.mapper != nullptr) {
    ImGui::Text("ROM BANK: %d", gb->bus.mapper->rom_bank);
    ImGui::Text("RAM BANK: %d", gb->bus.mapper->ram_bank);
  }
  ImGui::Separator();
  ImGui::Text("SPEED: %s", gb->cpu.speed == SPEED::DOUBLE ? "DOUBLE" : "NORMAL");
  ImGui::Separator();

  ImGui::Text("Z: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::ZERO));
  ImGui::Text("N: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::NEGATIVE));
  ImGui::Text("H: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::HALF_CARRY));
  ImGui::Text("C: %x", gb->cpu.get_flag(Umibozu::SM83::FLAG::CARRY));
  ImGui::Separator();
  ImGui::Text("pointer to cpu = %p", (void*)&gb->cpu);
  ImGui::Text("pc = 0x%x", gb->cpu.PC);
  if (gb->bus.mapper != nullptr) {
    ImGui::Text("opcode: 0x%x", gb->cpu.peek(gb->cpu.PC));
  }
  ImGui::Text("status: %s", gb->cpu.get_cpu_mode_string().c_str());
  ImGui::Separator();
  ImGui::Text("DIV = 0x%x", gb->bus.timer->get_div());
  ImGui::Text("TIMA = 0x%x", gb->bus.timer->counter);
  ImGui::Text("TMA = 0x%x", gb->bus.timer->modulo);
  ImGui::Text("timer enabled = %d", gb->bus.timer->ticking_enabled);
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
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  show_menubar();
  // if (state.controls_window_open) {
  //   show_controls_menu(&state.controls_window_open);
  // }

  if (state.texture_window_open) {
    show_viewport();
  }
  if (state.cpu_info_open) {
    show_cpu_info();
  }
  if (state.ppu_info_open) {
    show_ppu_info();
  }
  if (state.apu_info_open) {
    show_apu_info();
  }

  if (state.memory_viewer_open) {
    show_memory_viewer();
  }
  if (state.io_info_open) {
    show_io_info();
  }

  ImGui::Render();
  SDL_SetRenderScale(renderer, state.io->DisplayFramebufferScale.x, state.io->DisplayFramebufferScale.y);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  SDL_GetWindowSize(window, &screenWidth, &screenHeight);

  dst = {
      // crops the texture to 160x144 and stretches it to user window size
      .x = 0,
      .y = (float)ImGui::GetFrameHeight(),
      .w = (float)screenWidth,
      .h = (float)screenHeight,
  };

  SDL_UpdateTexture(state.ppu_texture, nullptr, gb->ppu.db.disp_buf, 256 * 2);

  SDL_RenderTexture(renderer, state.ppu_texture, &src, &dst);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
  SDL_RenderPresent(renderer);
  end_ticks = SDL_GetTicks();

  float frameTime = (end_ticks - start_ticks) / 1000.0f;  // in seconds

  if (frameTime > 0) {
    fps = 1.0f / frameTime;
    printf("FPS: %.2f\n", fps);
  }
  start_ticks = SDL_GetTicks();
}
void Frontend::show_viewport() {
  ImGui::Begin("PPU Texture", &state.texture_window_open, 0);
  ImGui::Text("pointer to texture = %p", (void*)&state.ppu_texture);
  ImGui::Text("pointer to gb instance = %p", (void*)&gb);
  ImGui::Text("fps = %f", state.io->Framerate);

  ImGui::Image((void*)state.ppu_texture, ImVec2(256 * 2, 256 * 2));

  ImGui::End();
}
Frontend::Frontend(GB* gb) : gb(gb) {
  assert(this->gb != nullptr);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
    fmt::println("ERROR: {}", SDL_GetError());
    exit(-1);
  }

  auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  this->window   = SDL_CreateWindow(fmt::format("umibozu-{}", SOFTWARE_VERSION).c_str(), 960, 540, window_flags);
  this->renderer = SDL_CreateRenderer(window, NULL);

  SDL_SetRenderVSync(renderer, 1);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  this->state.io = &io;

  ImGui::StyleColorsDark();

  this->state.ppu_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR1555, SDL_TEXTUREACCESS_TARGET, 256, 256);

  this->state.viewport = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR1555, SDL_TEXTUREACCESS_TARGET, 256, 256);

  init_audio_device();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
}

void Frontend::show_memory_viewer() {
  ImGui::Begin("Memory Viewer", &state.memory_viewer_open, 0);
  const char* regions[] = {
      "Region: [0x8000 - 0x9FFF] VRAM",     "Region: [0xA000 - 0xBFFF] EXT RAM", "Region: [0xC000 - 0xCFFF] WRAM Bank [0]", "Region: [0xD000 - 0xDFFF] WRAM Current Active Bank",
      "Region: [0xE000 - 0xFDFF] Echo RAM", "Region: [0xFE00 - 0xFE9F] OAM",     "Region: [0xFF80 - 0xFFFE] HRAM",
  };

  const void* memory_partitions[] = {
      gb->bus.vram->data(), gb->bus.cart->ext_ram.data(), gb->bus.wram_banks[0].data(), gb->bus.wram->data(), gb->bus.wram_banks[0].data(), gb->bus.oam.data(), gb->bus.hram.data(),
  };

  const size_t memory_partition_size[] = {
      gb->bus.vram->size(), gb->bus.cart->ext_ram.size(), gb->bus.wram_banks[0].size(), gb->bus.wram->size(), gb->bus.wram_banks[0].size(), gb->bus.oam.size(), gb->bus.hram.size(),
  };

  static int SelectedItem = 0;

  if (ImGui::Combo("Regions", &SelectedItem, regions, IM_ARRAYSIZE(regions))) {
    // SPDLOG_DEBUG("switched to: {}", regions[SelectedItem]);
  }
  editor_instance.OptShowAscii = false;
  editor_instance.ReadOnly     = true;

  editor_instance.DrawContents((void*)memory_partitions[SelectedItem], memory_partition_size[SelectedItem]);
  ImGui::End();
}

void Frontend::init_audio_device() {
  SDL_AudioSpec spec = {};
  spec.freq          = 48000;
  spec.format        = SDL_AUDIO_F32;
  spec.channels      = 2;
  stream             = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

  if (!stream) {
    fmt::println("Couldn't create audio stream: {}", SDL_GetError());
    exit(-1);
  }

  SDL_ResumeAudioStreamDevice(stream);
}

void Frontend::dump_framebuffer() {
  SDL_Log("dumped framebuffer to %s\n", "UNIMPL");
}