#include "timer.hpp"


void Timer::set_tac(const u8 value) {
    ticking_enabled = (value & 0x4) != 0;
}

void Timer::increment_div(const u8 value) {
    u16 new_div = get_full_div() + value;   

    // if(((get_div() & (1<<4)) != 0) && (((new_div >> 8) & (1<<4)) == 0)) {
    //     apu->frame_sequencer.step();
    // }

    div = new_div;
}

void Timer::reset_div() {
    // if((get_div() & (1<<5)) != 0) {
    //     apu->frame_sequencer.step();
    // }

    div = 0;
}