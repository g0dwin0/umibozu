#pragma once
#include <vector>

#include "common.h"
#include "core/cart/cart.h"
#include "core/cpu/cpu.h"
#include "mapper/mappers.h"

struct GB {
 private:
  SharpSM83 cpu;
  Bus bus;

 public:
  GB();
  
  void load_cart(std::vector<u8>);
  void init_hw_regs();
  void start(u64 count = 0xFFFFFFF);
};