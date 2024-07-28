#pragma once
#include "common.h"

#define SOUND_LENGTH_RATE 2
#define ENVELOPE_SWEEP_RATE 8
#define CH1_FREQ_SWEEP 4

enum DIRECTION { ADDITION, SUBTRACTION };
struct Registers {
  // Master
  union {
    u8 value = 0xF1;
    struct {
      u8 CH1_ON       : 1;
      u8 CH2_ON       : 1;
      u8 CH3_ON       : 1;
      u8 CH4_ON       : 1;
      u8              : 3;
      u8 AUDIO_ON_OFF : 1;
    };
  } NR52;  // Audio Master Control

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
  } NR51;  // Sound panning

  union {
    u8 value = 0x77;
    struct {
      u8 RIGHT_VOLUME : 3;
      u8 VIN_RIGHT    : 1;
      u8 LEFT_VOLUME  : 3;
      u8 VIN_LEFT     : 1;
    } registers;
  } NR50;  // Master volume & VIN panning

  // Channel 1

  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    u8 duty_step_counter;
    
    u8 volume_ctr;

    u32 sweep_shadow_reg;
    u16 period;  // 11 bit value; cover edge cases later
    u32 sweep_timer;
    bool sweep_enabled;

    union {
      u8 value = 0x80;
      struct {
        u8 SHIFT        : 3;
        u8 NEGATE       : 1;
        u8 SWEEP_PERIOD : 3;
        u8              : 1;
      };
    } NR10;  // Channel 1 sweep

    union {
      u8 value = 0xBF;
      struct {
        u8 INITIAL_LENGTH_TIMER : 6;
        u8 WAVE_DUTY            : 2;
      } registers;
    } NR11;  // Channel 1 length timer & duty cycle

    union {
      u8 value = 0xF3;
      struct {
        u8 SWEEP_PACE         : 3;
        u8 ENVELOPE_DIRECTION : 1;
        u8 INITIAL_VOLUME     : 4;
      } registers;
    } NR12;  // Channel 1 volume & envelope

    union {
      u8 value = 0xFF;
    } NR13;  // Channel 1 period low
    struct {
      union {
        u8 value = 0xBF;
        struct {
          u8 PERIOD        : 3;
          u8               : 3;
          u8 LENGTH_ENABLE : 1;
          u8 TRIGGER       : 1;
        } registers;
      };  // Channel 1 period high & control
    } NR14;

    bool get_dac_enabled() { return (NR12.value & 0xf8) != 0; }
  } CHANNEL_1;
  // Channel 2

  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    u32 duty_step_counter;

    union {
      u8 value = 0x3F;
      struct {
        u8 INITIAL_LENGTH_TIMER : 6;
        u8 WAVE_DUTY            : 2;
      } registers;
    } NR21;  // Channel 2 length timer & duty cycle

    union {
      u8 value = 0x00;
      struct {
        u8 SWEEP_PACE         : 3;
        u8 ENVELOPE_DIRECTION : 1;
        u8 INITIAL_VOLUME     : 4;
      } registers;
    } NR22;  // Channel 2 volume & envelope

    union {
      u8 value = 0xFF;
    } NR23;  // Channel 2 period low

    struct {
      union {
        u8 value = 0xBF;
        struct {
          u8 PERIOD        : 3;
          u8               : 3;
          u8 LENGTH_ENABLE : 1;
          u8 TRIGGER       : 1;
        } registers;
      };  // Channel 2 period high & control
    } NR24;
    bool get_dac_enabled() { return (NR22.value & 0xf8) != 0; }
  } CHANNEL_2;
  // Channel 3 - Wave output
  struct {
    bool channel_enabled = false;
    u16 length_timer     = 63;
    u8 duty_step_counter;
    u8 sample_index;

    union {
      u8 value = 0x7F;
      struct {
        u8            : 7;
        u8 DAC_ON_OFF : 1;
      } registers;
    } NR30;  // Channel 3 DAC enable

    union {
      u8 value = 0x9F;
      struct {
        u8              : 5;
        u8 output_level : 2;
        u8              : 1;
      } registers;
    } NR32;  // Channel 3 output level

    union {
      u8 value = 0xFF;
    } NR33;  // Channel 3 period low

    union {
      u8 value = 0xBF;
      struct {
        u8 PERIOD        : 3;
        u8               : 3;
        u8 LENGTH_ENABLE : 1;
        u8 TRIGGER       : 1;
      } registers;
    } NR34;  // Channel 3 period high & control
    bool get_dac_enabled() {
      return NR30.registers.DAC_ON_OFF == 1 ? true : false;
    }
  } CHANNEL_3;
  // Channel 4 - Noise
  struct {
    bool channel_enabled = false;
    u8 length_timer      = 63;
    u8 duty_step_counter;

    union {
      u8 value = 0xFF;
      struct {
        u8 INITIAL_LENGTH_TIMER : 6;
        u8                      : 2;
      } registers;
    } NR41;  // Channel 4 period high & control

    union {
      u8 value = 0x00;
      struct {
        u8 SWEEP_PACE         : 3;
        u8 ENVELOPE_DIRECTION : 1;
        u8 INITIAL_VOLUME     : 4;
      } registers;
    } NR42;  // Channel 4 volume & envelope

    union {
      u8 value = 0x00;
      struct {
        u8 CLOCK_DIVIDER : 3;
        u8 LFSR_WIDTH    : 1;
        u8 CLOCK_SHIFT   : 4;
      } registers;
    } NR43;  // Channel 4 frequency & randomness

    union {
      u8 value = 0xBF;
      struct {
        u8               : 6;
        u8 LENGTH_ENABLE : 1;
        u8 TRIGGER       : 1;
      } registers;
    } NR44;  // Channel 4 control
    bool get_dac_enabled() { return (NR42.value & 0xf8) != 0; }
  } CHANNEL_4;
};
struct FrameSequencer {
 private:
  u8 nStep        = 0;
  Registers* regs = nullptr;

 public:
  FrameSequencer(Registers* ptr) : regs(ptr) {
    if (regs == nullptr) {
      fmt::println("bad pointer to APU registers");
      exit(-1);
    }
  };

  void step();
  void reset();
  [[nodiscard]] u8 get_step();
};

class APU {
 private:
  Registers regs;

 public:
  void handle_write(u8 v, IO_REG r);
  FrameSequencer frame_sequencer = {&regs};
  void clear_apu_registers();
  bool is_allowed_reg(IO_REG r);
  [[nodiscard]] u8 handle_read(IO_REG r);
};