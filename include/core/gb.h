#pragma once
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
  void start();
};