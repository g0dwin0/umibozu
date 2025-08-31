#include "apu.hpp"

#include <cassert>

#include "SDL3/SDL_audio.h"
#include "bus.hpp"
#include "common.hpp"
#include "fmt/base.h"
#ifndef SYSTEM_TEST_MODE
#include "frontend/window.hpp"
#endif
bool APU::writable_when_off(IO_REG sndreg) {
  switch (sndreg) {
    case NR11:
    case NR21:
    case NR31:
    case NR41:
    case NR52: return true;
    default: return false;
  }
}

void APU::write(IO_REG sndreg, u8 value) {
  if (!regs.NR52.AUDIO_ON && writable_when_off(sndreg) == false) return;

  switch (sndreg) {
    case NR10: {
      regs.channel_1.NR10.value = value;

      if (regs.channel_1.NR10.direction_negate == 0 && regs.channel_1.sweep_calc_in_negate) {
        regs.channel_1.channel_enabled = false;
      }
      return;
    }
    case NR11: {
      if (regs.NR52.AUDIO_ON) regs.channel_1.NR11.value = value & 0b11000000;

      regs.channel_1.length_timer = 64 - (value & 0x3f);
      // fmt::println("CH1 NEW LENGTH TIMER: {}", regs.CHANNEL_1.length_timer);
      return;
    }
    case NR12: {
      regs.channel_1.NR12.value = value;
      if (!regs.channel_1.dac_enabled()) regs.channel_1.channel_enabled = false;
      return;
    }
    case NR13: {
      regs.channel_1.frequency &= ~0xFF;
      regs.channel_1.frequency |= value;
      return;
    }
    case NR14: {  // update upper 3 bits channel period hence 16 bit value (11 bits used) -- lower 8 get updated in NR13
      regs.channel_1.frequency &= ~0b11100000000;
      regs.channel_1.frequency |= ((value & 0b111) << 8);

      u8 old_enable_bit         = regs.channel_1.NR14.length_enable;
      regs.channel_1.NR14.value = value;

      if ((seq_current_step & 1) && (old_enable_bit == 0 && regs.channel_1.NR14.length_enable) && regs.channel_1.length_timer != 0) {
        regs.channel_1.length_timer--;
        fmt::println("ticked off rising edge ch 1: {}", regs.channel_1.length_timer);
        if (regs.channel_1.length_timer == 0) {
          regs.channel_1.channel_enabled = false;
        }
      }

      if ((value & (1 << 7)) != 0) {  // trigger
        ch1_trigger_event();
      }

      return;
    }
    case NR21: {
      if (regs.NR52.AUDIO_ON) regs.channel_2.NR21.value = value & 0b11000000;  // wave duty is only set if APU is on

      regs.channel_2.length_timer = 64 - (value & 0x3f);
      // fmt::println("channel 2 new length timer: {}", regs.CHANNEL_2.length_timer);

      return;
    }
    case NR22: {
      regs.channel_2.NR22.value = value;
      if (!regs.channel_2.dac_enabled()) regs.channel_2.channel_enabled = false;
      return;
    }
    case NR23: {
      regs.channel_2.frequency &= ~0xFF;
      regs.channel_2.frequency |= value;
      return;
    }
    case NR24: {  // period high & control

      regs.channel_2.frequency &= ~0b11100000000;
      regs.channel_2.frequency |= ((value & 0b111) << 8);
      u8 old_enable_bit         = regs.channel_2.NR24.length_enable;
      regs.channel_2.NR24.value = value;

      if ((seq_current_step & 1) && (old_enable_bit == 0 && regs.channel_2.NR24.length_enable) && regs.channel_2.length_timer != 0) {
        regs.channel_2.length_timer--;
        fmt::println("ticked off rising edge ch 2: {}", regs.channel_2.length_timer);
        if (regs.channel_2.length_timer == 0) {
          regs.channel_2.channel_enabled = false;
        }
      }

      if ((value & (1 << 7)) != 0) {  // channel is triggered, we start counting (only if APU is on)
        ch2_trigger_event();
      }

      return;
    }
    case NR30: {
      regs.channel_3.NR30.value = value;
      if (!regs.channel_3.dac_enabled()) regs.channel_3.channel_enabled = false;

      return;
    }
    case NR31: {
      regs.channel_3.length_timer = 256 - value;
      return;
    }
    case NR32: {  // output level
      regs.channel_3.NR32.value = value;
      return;
    }
    case NR33: {  // lower bits period
      regs.channel_3.NR33.value = value;
      return;
    }
    case NR34: {
      u8 old_enable_bit         = regs.channel_3.NR34.length_enable;
      regs.channel_3.NR34.value = value;

      if ((seq_current_step & 1) && (old_enable_bit == 0 && regs.channel_3.NR34.length_enable) && regs.channel_3.length_timer != 0) {
        regs.channel_3.length_timer--;
        fmt::println("ticked off rising edge ch 3: {}", regs.channel_3.length_timer);
        if (regs.channel_3.length_timer == 0 && (value & 1 << 7) == 0) {
          regs.channel_3.channel_enabled = false;
        }
      }

      if ((value & (1 << 7)) != 0) {  // trigger
        ch3_trigger_event();
      }

      return;
    }

    case NR41: {
      if (regs.NR52.AUDIO_ON) regs.channel_4.NR41.value = value & 0b11000000;

      regs.channel_4.length_timer = 64 - (value & 0x3f);
      return;
    }
    case NR42: {
      regs.channel_4.NR42.value = value;
      if (!regs.channel_4.dac_enabled()) regs.channel_4.channel_enabled = false;
      return;
    }
    case NR43: {
      regs.channel_4.NR43.value = value;
      return;
    }
    case NR44: {
      bool length_was_enabled   = regs.channel_4.NR44.length_enable;
      regs.channel_4.NR44.value = value;

      if ((seq_current_step & 1) == 1 && (!length_was_enabled && regs.channel_4.NR44.length_enable) && regs.channel_4.length_timer != 0) {
        regs.channel_4.length_timer--;
        // fmt::println("ticked off rising edge ch 4: {}", regs.CHANNEL_4.length_timer);
        if (regs.channel_4.length_timer == 0) {
          regs.channel_4.channel_enabled = false;
        }
      }

      if ((value & (1 << 7)) != 0) {  // trigger
        ch4_trigger_event();
      }

      return;
    }
    case NR50: {
      regs.NR50.value = value;
      return;
    }
    case NR51: {
      regs.NR51.value = value;
      return;
    }
    case NR52: {
      // falling edge disables APU
      u8 old_bit7     = regs.NR52.value & (1 << 7) ? 1 : 0;
      regs.NR52.value = value & (1 << 7);
      u8 new_bit7     = regs.NR52.value & (1 << 7) ? 1 : 0;

      if ((regs.NR52.value & (1 << 7)) == 0) {
        clear_apu_registers();
        fmt::println("disabled APU -- clearing registers");
      }

      // rising edge on enabled bit APU resets frame sequencer
      if (old_bit7 == 0 && new_bit7 == 1) {
        seq_current_step = 0;

        if (bus->mode == SYSTEM_MODE::CGB) {
          regs.channel_1.length_timer = 64;
          regs.channel_2.length_timer = 64;
          regs.channel_3.length_timer = 256;
          regs.channel_4.length_timer = 64;
        }
      }

      return;
    }
    default: {
      // fmt::println("[APU] invalid register write: {:#08x}", (u8)sndreg);
      return;
    }
  }
}

void APU::clear_apu_registers() {
  regs.channel_1.NR10.value      = 0;
  regs.channel_1.NR11.value      = 0;
  regs.channel_1.NR12.value      = 0;
  regs.channel_1.NR13.value      = 0;
  regs.channel_1.frequency       = 0;
  regs.channel_1.NR14.value      = 0;
  regs.channel_1.channel_enabled = false;

  regs.channel_2.NR21.value      = 0;
  regs.channel_2.NR22.value      = 0;
  regs.channel_2.NR23.value      = 0;
  regs.channel_2.NR24.value      = 0;
  regs.channel_2.frequency       = 0;
  regs.channel_2.channel_enabled = false;

  regs.channel_3.NR30.value      = 0;
  regs.channel_3.NR32.value      = 0;
  regs.channel_3.NR33.value      = 0;
  regs.channel_3.NR34.value      = 0;
  regs.channel_3.channel_enabled = false;

  // regs.channel_4.frequency       = 0;
  regs.channel_4.NR41.value      = 0;
  regs.channel_4.NR42.value      = 0;
  regs.channel_4.NR43.value      = 0;
  regs.channel_4.NR44.value      = 0;
  regs.channel_4.channel_enabled = false;

  regs.NR50.value = 0;
  regs.NR51.value = 0;

  // reset frame sequencer
  seq_current_step = 7;
};

u8 APU::read(IO_REG sndreg) {
  switch (sndreg) {
    case NR10: return regs.channel_1.NR10.value | 0x80;
    case NR11: return regs.channel_1.NR11.value | 0x3F;
    case NR12: return regs.channel_1.NR12.value;
    case NR13: return 0xFF;
    case NR14: return regs.channel_1.NR14.value | 0xBF;
    case NR21: return regs.channel_2.NR21.value | 0x3F;
    case NR22: return regs.channel_2.NR22.value;
    case NR23: return 0xFF;
    case NR24: return regs.channel_2.NR24.value | 0xBF;
    case NR30: return regs.channel_3.NR30.value | 0x7F;
    case NR31: return 0xFF;
    case NR32: return regs.channel_3.NR32.value | 0x9F;
    case NR33: return 0xFF;
    case NR34: return regs.channel_3.NR34.value | 0xBF;
    case NR41: return 0xFF;
    case NR42: return regs.channel_4.NR42.value;
    case NR43: return regs.channel_4.NR43.value;
    case NR44: return regs.channel_4.NR44.value | 0xBF;
    case NR50: return regs.NR50.value;
    case NR51: return regs.NR51.value;

    case NR52: {
      u8 retval = regs.NR52.value;

      retval &= ~0x0F;

      if (regs.channel_1.channel_enabled) retval |= (1 << 0);
      if (regs.channel_2.channel_enabled) retval |= (1 << 1);
      if (regs.channel_3.channel_enabled) retval |= (1 << 2);
      if (regs.channel_4.channel_enabled) retval |= (1 << 3);

      return retval | 0x70;
    }

    default: {
      fmt::println("[APU] invalid register read: {:#04x}", (u8)sndreg);
      return 0xFF;
    }
  }
}

void APU::step_seq() {
  if (!regs.NR52.AUDIO_ON) {
    // fmt::println("audio disabled, not stepping frame sequencer from {}", nStep);
    return;
  }

  if ((seq_current_step % 2) == 0) {  // tick length timers
    // fmt::println("ticking length timers nStep: {}", nStep);

    // ch 1
    if (regs.channel_1.NR14.length_enable && regs.channel_1.length_timer != 0) {
      //   fmt::println("incrementing length timer");
      regs.channel_1.length_timer--;
      // fmt::println("CHANNEL 1 LENGTH TIMER: {}", regs.CHANNEL_1.length_timer);
      if (regs.channel_1.length_timer == 0) {
        fmt::println("[APU] disabled channel 1");
        regs.channel_1.channel_enabled = false;
      }
    }

    // ch 2
    if (regs.channel_2.NR24.length_enable && regs.channel_2.length_timer != 0) {
      regs.channel_2.length_timer--;
      // fmt::println("CHANNEL 2 LENGTH TIMER: {}", regs.CHANNEL_2.length_timer);
      if (regs.channel_2.length_timer == 0) {
        fmt::println("[APU] disabled channel 2");
        regs.channel_2.channel_enabled = false;
      }
    }

    // ch 3
    if (regs.channel_3.NR34.length_enable && regs.channel_3.length_timer != 0) {
      regs.channel_3.length_timer--;
      // fmt::println("CHANNEL 3 LENGTH TIMER: {}", regs.CHANNEL_3.length_timer);
      if (regs.channel_3.length_timer == 0) {
        fmt::println("[APU] disabled channel 3");
        regs.channel_3.channel_enabled = false;
      }
    }

    // ch 4
    if (regs.channel_4.NR44.length_enable && regs.channel_4.length_timer != 0) {
      regs.channel_4.length_timer--;
      // fmt::println("CHANNEL 4 LENGTH TIMER: {}", regs.CHANNEL_4.length_timer);
      if (regs.channel_4.length_timer == 0) {
        fmt::println("[APU] disabled channel 4");
        regs.channel_4.channel_enabled = false;
      }
    }
  }

  if (seq_current_step == 2 || seq_current_step == 6) {  // CH1 freq sweep
    // tick_ch1_sweep();
    if (regs.channel_1.sweep_timer > 0) regs.channel_1.sweep_timer -= 1;

    if (regs.channel_1.sweep_timer == 0) {
      // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
      // The volume envelope and sweep timers treat a period of 0 as 8.
      regs.channel_1.sweep_timer = regs.channel_1.NR10.sweep_period != 0 ? regs.channel_1.NR10.sweep_period : 8;

      if (regs.channel_1.sweep_enabled && regs.channel_1.NR10.sweep_period != 0) {
        if (regs.channel_1.NR10.direction_negate == SUBTRACTION) regs.channel_1.sweep_calc_in_negate = true;

        // perform sweep calc
        u32 new_frequency = calculate_sweep_freq();

        // overflow check
        if (new_frequency > 0x7FF) {
          regs.channel_1.channel_enabled = false;
        }

        // update channel frequency
        if (new_frequency <= 0x7FF && regs.channel_1.NR10.sweep_shift != 0) {
          regs.channel_1.sweep_freq_shadow_reg = new_frequency;
          regs.channel_1.frequency             = new_frequency;
        }

        new_frequency = calculate_sweep_freq();

        // overflow check, on new frequency calculated using shadow register.
        if (new_frequency > 0x7FF) {
          regs.channel_1.channel_enabled = false;
        }
      }
    }
  };

  if (seq_current_step == 7) {  // Vol env
    tick_vol_env_ch1();
    tick_vol_env_ch2();
    tick_vol_env_ch4();
  }

  seq_current_step++;
  if (seq_current_step == 8) seq_current_step = 0;
}

u32 APU::calculate_sweep_freq() {
  u32 new_frequency = regs.channel_1.sweep_freq_shadow_reg >> regs.channel_1.NR10.sweep_shift;

  switch (regs.channel_1.NR10.direction_negate) {
    case ADDITION: {
      new_frequency = regs.channel_1.sweep_freq_shadow_reg + new_frequency;
      break;
    }
    case SUBTRACTION: {
      new_frequency = regs.channel_1.sweep_freq_shadow_reg - new_frequency;
      break;
    }
  }

  return new_frequency;
}
#ifndef SYSTEM_TEST_MODE
void APU::tick(u32 cycles) {
  regs.channel_1.frequency_timer -= cycles;
  regs.channel_2.frequency_timer -= cycles;
  // regs.channel_3.frequency_timer -= cycles;
  regs.channel_4.frequency_timer -= cycles;

  sample_rate -= cycles;

  if (regs.channel_1.frequency_timer <= 0) {
    regs.channel_1.duty_step = ((regs.channel_1.duty_step + 1) & 7);
    regs.channel_1.frequency_timer += (2048 - regs.channel_1.frequency) * 4;
  }

  if (regs.channel_2.frequency_timer <= 0) {
    regs.channel_2.duty_step = ((regs.channel_2.duty_step + 1) & 7);
    regs.channel_2.frequency_timer += (2048 - regs.channel_2.frequency) * 4;
  }

  if (regs.channel_4.frequency_timer <= 0) {
    regs.channel_4.frequency_timer = (regs.channel_4.NR43.clock_divider > 0 ? (regs.channel_4.NR43.clock_divider << 4) : 8) << regs.channel_4.NR43.clock_shift;

    u8 xor_result = (regs.channel_4.lfsr & 0b01) ^ ((regs.channel_4.lfsr & 0b10) >> 1);

    regs.channel_4.lfsr = (regs.channel_4.lfsr >> 1) | (xor_result << 14);

    if (regs.channel_4.NR43.lfsr_width == 1) {
      regs.channel_4.lfsr &= ~(1 << 6);
      regs.channel_4.lfsr |= (xor_result << 6);
    }
  }

  if (sample_rate <= 0) {
    sample();
    sample_rate += SAMPLE_RATE;
  }
}

f32 APU::sample_ch1() {
  if (!regs.channel_1.channel_enabled) return 0.0f;
  if (regs.channel_1.channel_volume == 0) return 0.0f;

  u8 duty_cycle_output = SQUARE_DUTY_WAVEFORMS.at(regs.channel_1.NR11.WAVE_DUTY).at(regs.channel_1.duty_step);
  return dac(duty_cycle_output * regs.channel_1.channel_volume, regs.channel_1.dac_enabled());
};

f32 APU::sample_ch2() {
  if (!regs.channel_2.channel_enabled) return 0.0f;
  if (regs.channel_2.channel_volume == 0) return 0.0f;
  u8 duty_cycle_output = SQUARE_DUTY_WAVEFORMS.at(regs.channel_2.NR21.wave_duty_pattern).at(regs.channel_2.duty_step);

  return dac(duty_cycle_output * regs.channel_2.channel_volume, regs.channel_2.dac_enabled());
};

f32 APU::sample_ch3() {
  if (!regs.channel_3.channel_enabled) return 0.0f;

  return 0.0f;
};
f32 APU::sample_ch4() {
  if (!regs.channel_4.channel_enabled || regs.channel_4.channel_volume == 0) return 0.0f;

  return dac((~(regs.channel_4.lfsr) & 0x01) * regs.channel_4.channel_volume, regs.channel_4.dac_enabled());
};

void APU::sample() {
  left_amp = right_amp = 0.0f;
  l_ch1_amp = r_ch1_amp = sample_ch1();
  l_ch2_amp = r_ch2_amp = sample_ch2();
  // l_ch3_amp = r_ch3_amp = 0;
  l_ch4_amp = r_ch4_amp = sample_ch4();

  if (!regs.NR51.CH1_LEFT) l_ch1_amp = 0.0f;
  if (!regs.NR51.CH1_RIGHT) r_ch1_amp = 0.0f;
  if (!regs.NR51.CH2_LEFT) l_ch2_amp = 0.0f;
  if (!regs.NR51.CH2_RIGHT) r_ch2_amp = 0.0f;
  if (!regs.NR51.CH3_LEFT) l_ch3_amp = 0.0f;
  if (!regs.NR51.CH3_RIGHT) r_ch3_amp = 0.0f;
  if (!regs.NR51.CH4_LEFT) l_ch4_amp = 0.0f;
  if (!regs.NR51.CH4_RIGHT) r_ch4_amp = 0.0f;
  assert(stream != nullptr);
  left_amp  = (l_ch1_amp + l_ch2_amp + l_ch3_amp + l_ch4_amp) / 4;
  right_amp = (r_ch1_amp + r_ch2_amp + r_ch3_amp + r_ch4_amp) / 4;

  //   left_amp  = 0.0f;
  // right_amp = 0.0f;

  // stream
  audio_buf[write_pos++] = left_amp;
  audio_buf[write_pos++] = right_amp;

  if (write_pos == 3072) {
    SDL_PutAudioStreamData(stream, audio_buf.data(), 3072 * sizeof(f32));

    write_pos = 0;
  }
}
bool APU::all_dacs_off() const { return !(regs.channel_1.dac_enabled() || regs.channel_2.dac_enabled() || regs.channel_3.dac_enabled() || regs.channel_4.dac_enabled()); }
f32 APU::dac(u8 in, bool dac_enabled) {
  assert(in <= 0xF);
  if (!dac_enabled) return 0.0f;

  return ((in / 7.5) - 1.0);
};
#endif
void APU::ch1_trigger_event() {
  // fmt::println("triggered ch1");

  // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Trigger_Event -- length ctr quirks
  if ((seq_current_step & 1) && regs.channel_1.NR14.length_enable && regs.channel_1.length_timer == 0) {
    regs.channel_1.length_timer = 63;
  }

  if (regs.channel_1.dac_enabled()) {
    regs.channel_1.channel_enabled = true;
    // fmt::println("[APU] triggered channel 1 (ON)");
  }

  if (regs.channel_1.length_timer == 0) {
    regs.channel_1.length_timer = 64;
  }

  // ====================== sweep specific trigger events ======================

  regs.channel_1.sweep_calc_in_negate = false;

  if (regs.channel_1.NR10.sweep_period != 0 || regs.channel_1.NR10.sweep_shift != 0) {
    regs.channel_1.sweep_enabled = true;
  } else {
    regs.channel_1.sweep_enabled = false;
  }

  // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
  // reload sweep timer: the volume envelope and sweep timers treat a period of 0 as 8
  regs.channel_1.sweep_timer = regs.channel_1.NR10.sweep_period != 0 ? regs.channel_1.NR10.sweep_period : 8;

  regs.channel_1.sweep_freq_shadow_reg = regs.channel_1.frequency;

  if (regs.channel_1.NR10.sweep_shift != 0) {
    if (regs.channel_1.NR10.direction_negate == SUBTRACTION) regs.channel_1.sweep_calc_in_negate = true;

    u32 new_frequency = calculate_sweep_freq();

    if (new_frequency > 2047) {
      fmt::println("freq: {:#010x} -- disabled channel", new_frequency);
      regs.channel_1.channel_enabled = false;
    }
  }

  regs.channel_1.frequency_timer = regs.channel_1.frequency;
  regs.channel_1.period_timer    = regs.channel_1.NR12.sweep_pace ? regs.channel_1.NR12.sweep_pace : 8;
  regs.channel_1.channel_volume  = regs.channel_1.NR12.initial_volume;
};
void APU::ch2_trigger_event() {
  // fmt::println("triggered channel 2");
  // Obscure behaviour
  if ((seq_current_step & 1) && regs.channel_2.NR24.length_enable && regs.channel_2.length_timer == 0) {
    regs.channel_2.length_timer = 63;
  }

  if (regs.channel_2.dac_enabled()) {
    regs.channel_2.channel_enabled = true;
    // fmt::println("[APU] triggered channel 2 (ON) -- loaded length timer: {}", regs.CHANNEL_2.length_timer);
  }

  if (regs.channel_2.length_timer == 0) {
    regs.channel_2.length_timer = 64;
    // fmt::println("[APU] post reload length timer: {}", regs.CHANNEL_2.length_timer);
  }

  regs.channel_2.frequency_timer = regs.channel_2.frequency;
  regs.channel_2.period_timer    = regs.channel_2.NR22.sweep_pace ? regs.channel_2.NR22.sweep_pace : 8;
  regs.channel_2.channel_volume  = regs.channel_2.NR22.initial_volume;
};
void APU::ch3_trigger_event() {  // Obscure behaviour
  if ((seq_current_step & 1) == 1 && regs.channel_3.NR34.length_enable && regs.channel_3.length_timer == 0) {
    regs.channel_3.length_timer = 255;
  }

  if (regs.channel_3.length_timer == 0) {
    regs.channel_3.length_timer = 256;
  }

  if (regs.channel_3.dac_enabled()) {
    regs.channel_3.channel_enabled = true;
    // fmt::println("[APU] triggered channel 3 (ON)");
  }
};
void APU::ch4_trigger_event() {
  // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Trigger_Event
  if ((seq_current_step & 1) && regs.channel_4.NR44.length_enable && regs.channel_4.length_timer == 0) {
    regs.channel_4.length_timer = 63;
  }

  if (regs.channel_4.length_timer == 0) {
    regs.channel_4.length_timer = 64;
  }

  if (regs.channel_4.dac_enabled()) {
    regs.channel_4.channel_enabled = true;
    // fmt::println("[APU] triggered channel 4 (ON)");
  }

  regs.channel_4.frequency_timer = (regs.channel_4.NR43.clock_divider > 0 ? (regs.channel_4.NR43.clock_divider << 4) : 8) << regs.channel_4.NR43.clock_shift;
  regs.channel_4.lfsr            = 0xFFFF;
  regs.channel_4.period_timer    = regs.channel_4.NR42.sweep_pace ? regs.channel_4.NR42.sweep_pace : 8;
  regs.channel_4.channel_volume  = regs.channel_4.NR42.initial_volume;
}

inline void APU::tick_vol_env_ch1() {
  if (regs.channel_1.NR12.sweep_pace != 0) {
    if (regs.channel_1.period_timer > 0) {
      regs.channel_1.period_timer -= 1;
    }

    if (regs.channel_1.period_timer == 0) {
      regs.channel_1.period_timer = regs.channel_1.NR12.sweep_pace;
      bool is_upwards             = regs.channel_1.NR12.envelope_direction == INCREASE;

      if ((regs.channel_1.channel_volume < 0xF && is_upwards) || regs.channel_1.channel_volume > 0x0 && !is_upwards) {
        if (is_upwards) {
          assert(regs.channel_1.channel_volume != 0xF);
          regs.channel_1.channel_volume += 1;
        } else {
          assert(regs.channel_1.channel_volume != 0);
          regs.channel_1.channel_volume -= 1;
        }
      }
    }
  }
}

inline void APU::tick_vol_env_ch2() {
  if (regs.channel_2.NR22.sweep_pace != 0) {
    if (regs.channel_2.period_timer > 0) {
      regs.channel_2.period_timer -= 1;
    }

    if (regs.channel_2.period_timer == 0) {
      regs.channel_2.period_timer = regs.channel_2.NR22.sweep_pace;
      bool is_upwards             = regs.channel_2.NR22.envelope_direction == INCREASE;

      if ((regs.channel_2.channel_volume < 0xF && is_upwards) || regs.channel_2.channel_volume > 0x0 && !is_upwards) {
        if (is_upwards) {
          regs.channel_2.channel_volume += 1;
        } else {
          regs.channel_2.channel_volume -= 1;
        }
      }
    }
  }
}
inline void APU::tick_vol_env_ch4() {
  if (regs.channel_4.NR42.sweep_pace != 0) {
    if (regs.channel_4.period_timer > 0) {
      regs.channel_4.period_timer -= 1;
    }

    if (regs.channel_4.period_timer == 0) {
      regs.channel_4.period_timer = regs.channel_4.NR42.sweep_pace;
      bool is_upwards             = regs.channel_4.NR42.envelope_direction == INCREASE;

      if ((regs.channel_4.channel_volume < 0xF && is_upwards) || regs.channel_4.channel_volume > 0x0 && !is_upwards) {
        if (is_upwards) {
          regs.channel_4.channel_volume += 1;
        } else {
          regs.channel_4.channel_volume -= 1;
        }
      }
    }
  }
}