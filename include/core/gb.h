#pragma once
#include "core/cart.h"
#include "core/cpu.h"

struct GB {
  SharpSM83 cpu;
  Bus bus;

  GB() { cpu.bus = &bus; }

  void init_hw_regs();
  void start();
};