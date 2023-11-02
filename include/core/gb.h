#pragma once
#include <string_view>
#include "core/cart/cart.h"
#include "core/cpu/cpu.h"

struct GB {
  SharpSM83 cpu;
  Bus bus;
  
  GB() {
    cpu.bus = &bus;
    init_hw_regs();
  }

  void init_hw_regs();
  void start(u64 count = 0xFFFFFFF);
};