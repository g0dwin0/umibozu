#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <vector>
#include "SDL3/SDL_audio.h"
struct Bus;
#include "bus.hpp"
#include "common.hpp"
#include "io_defs.hpp"

static constexpr u32 SAMPLE_RATE = 4194304 / 48000;

enum DIRECTION : u8 { ADDITION = 0, SUBTRACTION = 1 };
enum ENV_DIRECTION : u8 { DECREASE, INCREASE };

// https://gbdev.io/pandocs/Audio_Registers.html
// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware

struct Registers {
  union {
    u8 value = 0xF1;
    struct {
      bool CH1_ON   : 1;
      bool CH2_ON   : 1;
      bool CH3_ON   : 1;
      bool CH4_ON   : 1;
      u8            : 3;
      bool AUDIO_ON : 1;
    };
  } NR52;

  union {
    u8 value = 0xF3;
    struct {
      u8 CH1_RIGHT : 1;
      u8 CH2_RIGHT : 1;
      u8 CH3_RIGHT : 1;
      u8 CH4_RIGHT : 1;
      u8 CH1_LEFT  : 1;
      u8 CH2_LEFT  : 1;
      u8 CH3_LEFT  : 1;
      u8 CH4_LEFT  : 1;
    };
  } NR51;

  union {
    u8 value = 0x77;
    struct {
      u8 RIGHT_VOLUME : 3;
      u8 VIN_RIGHT    : 1;
      u8 LEFT_VOLUME  : 3;
      u8 VIN_LEFT     : 1;
    };
  } NR50;

  // channel 1
  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    u8 duty_step         = 0;

    i32 frequency_timer;

    u32 sweep_freq_shadow_reg;
    u16 frequency;  // frequency/period -- 11 bit value; cover edge cases later

    u8 channel_volume;
    i32 period_timer = 0;  // relates to vol env
    u32 sweep_timer;
    bool sweep_enabled;

    // Clearing the sweep negate mode bit in NR10 after at least one sweep calculation has been made using the negate mode since the last trigger causes the channel to be immediately disabled.
    // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
    bool sweep_calc_in_negate = false;

    union {
      u8 value = 0x80;
      struct {
        u8 sweep_shift             : 3;
        DIRECTION direction_negate : 1;
        u8 sweep_period            : 3;
        u8                         : 1;
      };
    } NR10;

    union {
      u8 value = 0xBF;
      struct {
        u8 INITIAL_LENGTH_TIMER : 6;
        u8 WAVE_DUTY            : 2;
      };
    } NR11;

    union {
      u8 value = 0xF3;
      struct {
        u8 sweep_pace                    : 3;
        ENV_DIRECTION envelope_direction : 1;
        u8 initial_volume                : 4;
      };
    } NR12;

    union {
      u8 value = 0xFF;
    } NR13;

    union {
      u8 value = 0xBF;
      struct {
        u8 period        : 3;
        u8               : 3;
        u8 length_enable : 1;
        u8 trigger       : 1;
      };
    } NR14;

    bool dac_enabled() const { return (NR12.value & 0xf8) != 0; }
  } channel_1;

  // channel 2
  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    i32 frequency_timer  = 0;
    u8 duty_step         = 0;
    i32 vol_env_timer    = 0;
    // frequency and period
    u16 frequency;
    i32 period_timer = 0;

    u8 channel_volume;

    union {
      u8 value = 0x3F;
      struct {
        u8 initial_length_timer : 6;
        u8 wave_duty_pattern    : 2;
      };
    } NR21;

    union {
      u8 value = 0x00;
      struct {
        u8 sweep_pace                    : 3;
        ENV_DIRECTION envelope_direction : 1;
        u8 initial_volume                : 4;
      };
    } NR22;

    union {
      u8 value = 0xFF;
    } NR23;

    union {
      u8 value = 0xBF;
      struct {
        u8 period          : 3;
        u8                 : 3;
        bool length_enable : 1;
        u8 trigger         : 1;
      };
    } NR24;

    bool dac_enabled() const { return (NR22.value & 0xf8) != 0; }
  } channel_2;

  // channel 3 - wave
  struct {
    bool channel_enabled = false;
    u16 length_timer     = 63;
    u8 channel_volume;
    u32 frequency_timer = 0;
    u32 period          = 0;
    u8 sample_index     = 0;

    union {
      u8 value = 0x7F;
      struct {
        u8          : 7;
        bool DAC_ON : 1;
      };
    } NR30;

    union {
      u8 value = 0x9F;
      struct {
        u8              : 5;
        u8 output_level : 2;
        u8              : 1;
      };
    } NR32;

    union {
      u8 value = 0xFF;
    } NR33;

    union {
      u8 value = 0xBF;
      struct {
        u8 period        : 3;
        u8               : 3;
        u8 length_enable : 1;
        u8 trigger       : 1;
      };
    } NR34;
    bool dac_enabled() const { return NR30.DAC_ON; };
  } channel_3;

  // channel 4 - noise
  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    i32 frequency_timer  = 0;

    u8 channel_volume;
    i32 period_timer = 0;  // relates to vol env
    u16 lfsr;

    union {
      u8 value = 0xFF;
      struct {
        u8 initial_length_timer : 6;
        u8                      : 2;
      };
    } NR41;

    union {
      u8 value = 0x00;
      struct {
        u8 sweep_pace                    : 3;
        ENV_DIRECTION envelope_direction : 1;
        u8 initial_volume                : 4;
      };
    } NR42;

    union {
      u8 value = 0x00;
      struct {
        u8 clock_divider : 3;
        u8 lfsr_width    : 1;
        u8 clock_shift   : 4;
      };
    } NR43;

    union {
      u8 value = 0xBF;
      struct {
        u8               : 6;
        u8 length_enable : 1;
        u8 trigger       : 1;
      };
    } NR44;
    bool dac_enabled() const { return (NR42.value & 0xf8) != 0; }
  } channel_4;
};

struct APU {
  APU() : audio_buf(32767) {
    std::fill(audio_buf.begin(), audio_buf.end(), 0.0f);
  };

  Registers regs;
  void write(IO_REG reg, u8 value);
  [[nodiscard]] u8 read(IO_REG r);

  void clear_apu_registers();

  // returns true the register can be written to when the APU is off, otherwise returns false.
  bool writable_when_off(IO_REG sndreg);

  u32 calculate_sweep_freq();

  // current step of the frame sequencer
  u8 seq_current_step = 0;

  void step_seq();
  void reset_seq();

  Bus* bus = nullptr;

  void tick(u32 cycles);

  const std::array<std::array<u8, 8>, 4> SQUARE_DUTY_WAVEFORMS = {
      {
       {0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 1, 1},
       {1, 0, 0, 0, 0, 1, 1, 1},
       {0, 1, 1, 1, 1, 1, 1, 0},
       }
  };

  u32 write_pos = 0;
  std::vector<f32> audio_buf;
  
  #ifndef SYSTEM_TEST_MODE
  f32 sample_ch1();
  f32 sample_ch2();
  f32 sample_ch3();
  f32 sample_ch4();

  void sample();
  f32 dac(u8 in, bool dac_enabled);
  bool all_dacs_off() const;
  SDL_AudioStream* stream = nullptr;
  #endif

  i32 sample_rate = SAMPLE_RATE;

  f32 left_amp, right_amp;
  f32 l_ch1_amp, r_ch1_amp;
  f32 l_ch2_amp, r_ch2_amp;
  f32 l_ch3_amp, r_ch3_amp;
  f32 l_ch4_amp, r_ch4_amp;

  void ch1_trigger_event();
  void ch2_trigger_event();
  void ch3_trigger_event();
  void ch4_trigger_event();

  void tick_vol_env_ch1();
  void tick_vol_env_ch2();
  void tick_vol_env_ch3();
  void tick_vol_env_ch4();

  
};