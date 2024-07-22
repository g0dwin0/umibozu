#include "apu.h"

#include <SDL2/SDL_audio.h>

#include "common.h"

bool APU::is_allowed_reg(IO_REG r) {
  if (r == NR52) return true;

  return false;
}

void APU::handle_write(u8 v, IO_REG r) {
  fmt::println("[APU] writing {:#04x} to {:4X}", v, 0xFF00 + (u8)r);
  // fmt::println("[APU] audio status: {}", (u8)regs.NR52.AUDIO_ON_OFF);
  // fmt::println("r 1 dac: {}", regs.CHANNEL_1.get_dac_enabled());
  //   fmt::println("r 2 dac: {}", regs.CHANNEL_2.get_dac_enabled());
  //   fmt::println("r 3 dac: {}", regs.CHANNEL_3.get_dac_enabled());
  //   fmt::println("r 4 dac: {}", regs.CHANNEL_4.get_dac_enabled());
  if (regs.NR52.AUDIO_ON_OFF == false && is_allowed_reg(r) == false) return;

  switch (r) {
    case NR10: {
      regs.CHANNEL_1.NR10.value = v;
      return;
    }
    case NR11: {
      regs.CHANNEL_1.NR11.value = v & 0b11000000;

      regs.CHANNEL_1.length_timer = (v & 0x3f);
      fmt::println("length timer 1: {}", regs.CHANNEL_1.length_timer);
      if (regs.CHANNEL_1.length_timer == 0) {
        fmt::println("length timer 1n: {}", regs.CHANNEL_1.length_timer);
      }
      return;
    }
    case NR12: {
      regs.CHANNEL_1.NR12.value = v;
      if (!regs.CHANNEL_1.get_dac_enabled())
        regs.CHANNEL_1.channel_enabled = false;
      return;
    }
    case NR13: {
      regs.CHANNEL_1.period &= ~0b00011111111;

      regs.CHANNEL_1.period |= v;

      return;
    }
    case NR14: {
      fmt::println("[NR14] {:08b}", v);

      regs.CHANNEL_1.period &= ~0b11100000000;
      regs.CHANNEL_1.period |= ((v & 0b111) << 8);


      u8 old_enable_bit         = regs.CHANNEL_1.NR14.registers.LENGTH_ENABLE;
      u8 p                      = (v | 0b10111111);
      regs.CHANNEL_1.NR14.value = p;

      if ((frame_sequencer.get_step() & 1) == 1 &&
          (old_enable_bit == 0 &&
           regs.CHANNEL_1.NR14.registers.LENGTH_ENABLE) &&
          regs.CHANNEL_1.length_timer != 64) {
        regs.CHANNEL_1.length_timer++;
        fmt::println("ticked off rising edge ch 1: {}",
                     regs.CHANNEL_1.length_timer);
        if (regs.CHANNEL_1.length_timer == 64) {
          regs.CHANNEL_1.channel_enabled = false;
        }
      }

      if ((v & (1 << 7)) != 0) {  // trigger
        // Obscure behaviour
        if ((frame_sequencer.get_step() & 1) == 1 &&
            regs.CHANNEL_1.NR14.registers.LENGTH_ENABLE &&
            regs.CHANNEL_1.length_timer == 64) {
          regs.CHANNEL_1.length_timer = 1;
        }

        if (regs.CHANNEL_1.get_dac_enabled()) {
          regs.CHANNEL_1.channel_enabled = true;
          fmt::println("[APU] triggered channel 1 (ON)");
        }
        if (regs.CHANNEL_1.length_timer == 64) {
          regs.CHANNEL_1.length_timer = 0;
        }
      }

      return;
    }
    case NR21: {
      regs.CHANNEL_2.NR21.value = v & 0b11000000;

      regs.CHANNEL_2.length_timer = (v & 0x3f);
      fmt::println("length timer 2: {}", regs.CHANNEL_2.length_timer);
      if (regs.CHANNEL_2.length_timer == 0) {
        fmt::println("length timer 2n: {}", regs.CHANNEL_2.length_timer);
      }
      // exit(0);
      return;
    }
    case NR22: {
      regs.CHANNEL_2.NR22.value = v;
      if (!regs.CHANNEL_2.get_dac_enabled())
        regs.CHANNEL_2.channel_enabled = false;
      return;
    }
    case NR23: {
      regs.CHANNEL_2.NR23.value = v;
      return;
    }
    case NR24: {
      fmt::println("[NR24] {:08b}", v);
      u8 old_enable_bit         = regs.CHANNEL_2.NR24.registers.LENGTH_ENABLE;
      u8 p                      = (v | 0b10111111);
      regs.CHANNEL_2.NR24.value = p;

      if ((frame_sequencer.get_step() & 1) == 1 &&
          (old_enable_bit == 0 &&
           regs.CHANNEL_2.NR24.registers.LENGTH_ENABLE) &&
          regs.CHANNEL_2.length_timer != 64) {
        regs.CHANNEL_2.length_timer++;
        fmt::println("ticked off rising edge ch 2: {}",
                     regs.CHANNEL_2.length_timer);
        if (regs.CHANNEL_2.length_timer == 64) {
          regs.CHANNEL_2.channel_enabled = false;
        }
      }

      if ((v & (1 << 7)) != 0) {  // trigger
        // Obscure behaviour
        if ((frame_sequencer.get_step() & 1) == 1 &&
            regs.CHANNEL_2.NR24.registers.LENGTH_ENABLE &&
            regs.CHANNEL_2.length_timer == 64) {
          regs.CHANNEL_2.length_timer = 1;
        }

        if (regs.CHANNEL_2.get_dac_enabled()) {
          regs.CHANNEL_2.channel_enabled = true;
          fmt::println("[APU] triggered channel 2 (ON)");
        }
        if (regs.CHANNEL_2.length_timer == 64) {
          regs.CHANNEL_2.length_timer = 0;
        }
      }

      return;
    }
    case NR30: {
      regs.CHANNEL_3.NR30.value = (v | 0b01111111);
      if (!regs.CHANNEL_3.get_dac_enabled())
        regs.CHANNEL_3.channel_enabled = false;

      return;
    }
    case NR31: {
      //  regs.CHANNEL_3.NR11.value = v & 0b11000000;
      fmt::println("chn 3 commited: {}", v);
      regs.CHANNEL_3.length_timer = v;
      fmt::println("length timer 3: {}", regs.CHANNEL_3.length_timer);
      if (regs.CHANNEL_3.length_timer == 0) {
        fmt::println("length timer 3n: {}", regs.CHANNEL_3.length_timer);
      }
      return;
    }
    case NR32: {
      regs.CHANNEL_3.NR32.value = v;
      return;
    }
    case NR33: {
      regs.CHANNEL_3.NR33.value = v;
      return;
    }
    case NR34: {
      fmt::println("[NR34] {:08b}", v);
      u8 old_enable_bit         = regs.CHANNEL_3.NR34.registers.LENGTH_ENABLE;
      u8 p                      = (v | 0b10111111);
      regs.CHANNEL_3.NR34.value = p;

      if ((frame_sequencer.get_step() & 1) == 1 &&
          (old_enable_bit == 0 &&
           regs.CHANNEL_3.NR34.registers.LENGTH_ENABLE) &&
          regs.CHANNEL_3.length_timer != 256) {
        regs.CHANNEL_3.length_timer++;
        fmt::println("ticked off rising edge ch 3: {}",
                     regs.CHANNEL_3.length_timer);
        if (regs.CHANNEL_3.length_timer == 256) {
          regs.CHANNEL_3.channel_enabled = false;
        }
      }

      if ((v & (1 << 7)) != 0) {  // trigger
        // Obscure behaviour
        if ((frame_sequencer.get_step() & 1) == 1 &&
            regs.CHANNEL_3.NR34.registers.LENGTH_ENABLE &&
            regs.CHANNEL_3.length_timer == 256) {
          regs.CHANNEL_3.length_timer = 1;
        }

        if (regs.CHANNEL_3.get_dac_enabled()) {
          regs.CHANNEL_3.channel_enabled = true;
          fmt::println("[APU] triggered channel 3 (ON)");
        }
        if (regs.CHANNEL_3.length_timer == 256) {
          regs.CHANNEL_3.length_timer = 0;
        }
      }

      return;
    }

    case NR41: {
      regs.CHANNEL_4.NR41.value = v & 0b11000000;

      regs.CHANNEL_4.length_timer = (v & 0x3f);
      fmt::println("length timer 4: {}", regs.CHANNEL_4.length_timer);
      if (regs.CHANNEL_4.length_timer == 0) {
        fmt::println("length timer 4n: {}", regs.CHANNEL_4.length_timer);
      }
      return;
    }
    case NR42: {
      regs.CHANNEL_4.NR42.value = v;
      if (!regs.CHANNEL_4.get_dac_enabled()) {
        regs.CHANNEL_4.channel_enabled = false;
      }
      return;
    }
    case NR43: {
      regs.CHANNEL_4.NR43.value = v;
      return;
    }
    case NR44: {
      fmt::println("[NR44] {:08b}", v);
      u8 old_enable_bit         = regs.CHANNEL_4.NR44.registers.LENGTH_ENABLE;
      u8 p                      = (v | 0b10111111);
      regs.CHANNEL_4.NR44.value = p;

      if ((frame_sequencer.get_step() & 1) == 1 &&
          (old_enable_bit == 0 &&
           regs.CHANNEL_4.NR44.registers.LENGTH_ENABLE) &&
          regs.CHANNEL_4.length_timer != 64) {
        regs.CHANNEL_4.length_timer++;
        fmt::println("ticked off rising edge ch 4: {}",
                     regs.CHANNEL_4.length_timer);
        if (regs.CHANNEL_4.length_timer == 64) {
          regs.CHANNEL_4.channel_enabled = false;
        }
      }

      if ((v & (1 << 7)) != 0) {  // trigger
        // Obscure behaviour
        if ((frame_sequencer.get_step() & 1) == 1 &&
            regs.CHANNEL_4.NR44.registers.LENGTH_ENABLE &&
            regs.CHANNEL_4.length_timer == 64) {
          regs.CHANNEL_4.length_timer = 1;
        }

        if (regs.CHANNEL_4.get_dac_enabled()) {
          regs.CHANNEL_4.channel_enabled = true;
          fmt::println("[APU] triggered channel 4 (ON)");
          if (regs.CHANNEL_4.length_timer == 64) {
            regs.CHANNEL_4.length_timer = 0;
          }
        }
      }

      return;
    }
    case NR50: {
      regs.NR50.value = v;
      fmt::println("[APU] Master volume / VIN: {:08b}", v);
      return;
    }
    case NR51: {
      regs.NR51.value = v;
      fmt::println("[APU] Sound panning: {:08b}", v);
      // fmt::println()
      return;
    }
    case NR52: {
      regs.NR52.value = v & (1 << 7);
      if ((regs.NR52.value & (1 << 7)) == 0) { clear_apu_registers(); }

      fmt::println("[APU] Audio value: {}", (u8)regs.NR52.value);
      fmt::println("[APU] Audio status: {}", regs.NR52.AUDIO_ON_OFF ? 1 : 0);
      regs.NR52.value |= 0b01110000;

      return;
    }
    default: {
      fmt::println("[APU] invalid register write: {:#08x}", (u8)r);
      // if((u8)r == 0x15) return;
      // exit(-1);
      return;
    }
  }
}

void APU::clear_apu_registers() {
  regs.CHANNEL_1.NR10.value      = 0;
  regs.CHANNEL_1.NR11.value      = 0;
  regs.CHANNEL_1.NR12.value      = 0;
  regs.CHANNEL_1.NR13.value      = 0;
  regs.CHANNEL_1.NR14.value      = 0;
  regs.CHANNEL_1.channel_enabled = false;

  regs.CHANNEL_2.NR21.value      = 0;
  regs.CHANNEL_2.NR22.value      = 0;
  regs.CHANNEL_2.NR23.value      = 0;
  regs.CHANNEL_2.NR24.value      = 0;
  regs.CHANNEL_2.channel_enabled = false;

  regs.CHANNEL_3.NR30.value      = 0;
  regs.CHANNEL_3.NR32.value      = 0;
  regs.CHANNEL_3.NR33.value      = 0;
  regs.CHANNEL_3.NR34.value      = 0;
  regs.CHANNEL_3.channel_enabled = false;

  regs.CHANNEL_4.NR41.value      = 0;
  regs.CHANNEL_4.NR42.value      = 0;
  regs.CHANNEL_4.NR43.value      = 0;
  regs.CHANNEL_4.NR44.value      = 0;
  regs.CHANNEL_4.channel_enabled = false;

  regs.NR50.value = 0;
  regs.NR51.value = 0;
};

u8 APU::handle_read(IO_REG r) {
  // fmt::println("[APU] reading {:#04x} to {:4X}", v, 0xFF00+(u8)r);
  // if(regs.NR52.AUDIO_ON_OFF )
  switch (r) {
    case NR10: {
      return regs.CHANNEL_1.NR10.value | 0x80;
    }
    case NR11: {
      return regs.CHANNEL_1.NR11.value | 0b00111111;
    }
    case NR12: {
      return regs.CHANNEL_1.NR12.value;
    }
    case NR13: {
      return 0xFF;
    }
    case NR14: {
      fmt::println("READING FROM APU NR14: {}",
                   regs.CHANNEL_1.NR14.value | 0b10111111);
      return regs.CHANNEL_1.NR14.value | 0b10111111;
    }
    case NR21: {
      return regs.CHANNEL_2.NR21.value | 0b00111111;
    }
    case NR22: {
      return regs.CHANNEL_2.NR22.value;
    }
    case NR23: {
      return 0xFF;
    }
    case NR24: {
      return regs.CHANNEL_2.NR24.value | 0b10111111;
    }
    case NR30: {
      return regs.CHANNEL_3.NR30.value | 0b01111111;
    }
    case NR31: {
      return 0xFF;
    }
    case NR32: {
      return regs.CHANNEL_3.NR32.value | 0b10011111;
    }
    case NR33: {
      return 0xFF;
    }
    case NR34: {
      return regs.CHANNEL_3.NR34.value | 0b10111111;
    }
    case NR41: {
      return 0xFF;
    }
    case NR42: {
      return regs.CHANNEL_4.NR42.value;
    }
    case NR43: {
      return regs.CHANNEL_4.NR43.value;
    }
    case NR44: {
      return regs.CHANNEL_4.NR44.value | 0b10111111;
    }
    case NR50: {
      return regs.NR50.value;
    }
    case NR51: {
      return regs.NR51.value;
    }
    case NR52: {
      u8 r_v = regs.NR52.value;
      // fmt::println("before settings chans: {:08b}", r_v);
      r_v &= ~0b00001111;
      // fmt::println("before settings chans 2: {:08b}", r_v);
      // fmt::println("regs.CHANNEL_1.channel_enabled: {}",
      //              regs.CHANNEL_1.channel_enabled);
      // fmt::println("regs.CHANNEL_2.channel_enabled: {}",
      //              regs.CHANNEL_2.channel_enabled);
      // fmt::println("regs.CHANNEL_3.channel_enabled: {}",
      //              regs.CHANNEL_3.channel_enabled);
      // fmt::println("regs.CHANNEL_4.channel_enabled: {}",
      //              regs.CHANNEL_4.channel_enabled);

      if (regs.CHANNEL_1.channel_enabled) r_v |= (1 << 0);
      if (regs.CHANNEL_2.channel_enabled) r_v |= (1 << 1);
      if (regs.CHANNEL_3.channel_enabled) r_v |= (1 << 2);
      if (regs.CHANNEL_4.channel_enabled) r_v |= (1 << 3);
      // fmt::println("before settings chans 3: {:08b}", r_v);
      return r_v;
    }

    default: {
      fmt::println("[APU] invalid register read: {:#04x}", (u8)r);
      return 0xFF;
    }
  }
}

void FrameSequencer::step() {
  if ((nStep % 2) == 0) {  // Sound length
    // fmt::println("step frame sequencer");
    if (regs->CHANNEL_1.NR14.registers.LENGTH_ENABLE &&
        regs->CHANNEL_1.length_timer != 64) {
      regs->CHANNEL_1.length_timer++;
      fmt::println("CHANNEL 1 LENGTH TIMER: {}", regs->CHANNEL_1.length_timer);
      if (regs->CHANNEL_1.length_timer == 64) {
        fmt::println("[APU] disabled channel 1");
        regs->CHANNEL_1.channel_enabled = false;
        // regs->CHANNEL_1.length_timer = 0;
      }
    }

    if (regs->CHANNEL_2.NR24.registers.LENGTH_ENABLE &&
        regs->CHANNEL_2.length_timer != 64) {
      regs->CHANNEL_2.length_timer++;
      // fmt::println("CHANNEL 2 LENGTH TIMER: {}",
      // regs->CHANNEL_2.length_timer);
      if (regs->CHANNEL_2.length_timer == 64) {
        fmt::println("[APU] disabled channel 2");
        regs->CHANNEL_2.channel_enabled = false;
        // regs->CHANNEL_1.length_timer = 0;
      }
    }

    if (regs->CHANNEL_3.NR34.registers.LENGTH_ENABLE &&
        regs->CHANNEL_3.length_timer != 256) {
      regs->CHANNEL_3.length_timer++;
      // fmt::println("CHANNEL 3 LENGTH TIMER: {}",
      // regs->CHANNEL_3.length_timer);
      if (regs->CHANNEL_3.length_timer == 256) {
        fmt::println("[APU] disabled channel 3");
        regs->CHANNEL_3.channel_enabled = false;
      }
    }
    // fmt::println("regs->CHANNEL_4.NR44.registers.LENGTH_ENABLE: {}",
    // regs->CHANNEL_4.NR44.registers.LENGTH_ENABLE != 0 ? "true" : "false");
    if (regs->CHANNEL_4.NR44.registers.LENGTH_ENABLE &&
        regs->CHANNEL_4.length_timer != 64) {
      regs->CHANNEL_4.length_timer++;
      fmt::println("CHANNEL 4 LENGTH TIMER: {}", regs->CHANNEL_4.length_timer);
      if (regs->CHANNEL_4.length_timer == 64) {
        fmt::println("[APU] disabled channel 4");
        regs->CHANNEL_4.channel_enabled = false;
      }
    }
  }

  if ((nStep == 2 || nStep == 4)) {  // CH1 freq sweep

  }

  // The volume envelope and sweep timers treat a period of 0 as 8.
  if (nStep == 7) {  // Vol env
  }

  nStep++;
  if (nStep == 8) nStep = 0;
};

u8 FrameSequencer::get_step() { return nStep; }

/*

> 07-len sweep period sync: #5 "Powering up APU MODs next frame time with 8192"

blargg's code was a bit misleading, but I was able to decipher it from observing
the same bit-twiddling in gambatte.

You want to clear the sequencer phase (0-7) back to zero only on a 0->1
transition of NR52.d7 If you clear it regardless (eg 0->0 or 1->1), the test
fails.

*/