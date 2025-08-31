#include "timer.hpp"

void Timer::set_tac(const u8 value) { ticking_enabled = (value & 0x4) != 0; }

void Timer::increment_div(const u8 value, bool is_cgb_double_speed) {
  u16 new_div = get_full_div() + value;
  assert(bus != nullptr);
  assert(bus->apu != nullptr);

//   if (is_double_speed) {
    
//     if ((get_div() & (1 << 6)) != 0) {
//       bus->apu->frame_sequencer.step();
//     }

//   } else {
    
    if (  (get_div() & (1 << 4)) != 0  && ((new_div >> 8) & (1 << 4)) == 0) {
      bus->apu->step_seq();
    }
    
//   }

  div = new_div;
}

void Timer::reset_div(bool is_double_speed) {
  // if (is_double_speed) {
  //   if ((get_div() & (1 << 6)) != 0) {
  //     bus->apu->frame_sequencer.step();
  //   }
  // } else {
    if ((get_div() & (1 << 4)) != 0) {
      bus->apu->step_seq();
     }
  // }

  div = 0;
}