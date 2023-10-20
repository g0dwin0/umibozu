#pragma once
#include "common.h"
#include "core/cpu.h"
#include "core/ops.h"
namespace Umibozu {
    
  typedef void (*func_ptr)(SharpSM83* cpu);

  struct Instruction {
    std::string name;
    func_ptr func;
    u8 m_cycles;
  };
  inline std::array<Instruction, 0xFF> lookup_table = {
      {"NOP", &NOP, 4}
  };
}  // namespace Umibozu