#pragma once
#include <vector>
#include "bus.h"
#include "common.h"
#include "cart.h"
#include "cpu.h"
#include "mappers.h"

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