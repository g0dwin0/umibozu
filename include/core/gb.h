#pragma once
#include <limits>
#include <vector>

#include "bus.h"
#include "cart.h"
#include "common.h"
#include "cpu.h"
#include "mappers.h"

struct GB {
 private:
  SharpSM83 cpu;
 public:
  Bus bus;
  GB();

  void load_cart(std::vector<u8>);
  void init_hw_regs();
  void start(u64 count = std::numeric_limits<u64>::max());
};