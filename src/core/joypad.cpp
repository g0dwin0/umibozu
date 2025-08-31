#include "core/joypad.hpp"

#include "common.hpp"
u8 Joypad::get_buttons() const { return A + (B << 1) + (SELECT << 2) + (START << 3); }

u8 Joypad::get_dpad() const { return RIGHT + (LEFT << 1) + (UP << 2) + (DOWN << 3); }