#pragma once
#include "common.hpp"

struct Joypad {
  u8 A      : 1 = 1;
  u8 B      : 1 = 1;
  u8 SELECT : 1 = 1;
  u8 START  : 1 = 1;

  u8 RIGHT  : 1 = 1;
  u8 LEFT   : 1 = 1;
  u8 UP     : 1 = 1;
  u8 DOWN   : 1 = 1;

  [[nodiscard]] u8 get_buttons() const {
    return A + (B << 1) + (SELECT << 2) + (START << 3);
  }

  [[nodiscard]] u8 get_dpad() const {
    return RIGHT + (LEFT << 1) + (UP << 2) + (DOWN << 3);
  }
};
enum BUTTONS { B, A, SELECT, START, RIGHT, LEFT, UP, DOWN, NONE = 0xFF };

[[nodiscard]] inline std::string get_button_name_from_enum(BUTTONS b) {
  switch (b) {
    case A: {
      return "A";
    }
    case B: {
      return "B";
    }
    case SELECT: {
      return "SELECT";
    }
    case START: {
      return "START";
    }
    case RIGHT: {
      return "RIGHT";
    }
    case LEFT: {
      return "LEFT";
    }
    case UP: {
      return "UP";
    }
    case DOWN: {
      return "DOWN";
    }
    default: {
      return "INVALID";
    }
  };
}
