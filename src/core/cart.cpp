#include "cart.hpp"

#include <span>
#include <sstream>
#include <vector>

#include "cart_constants.hpp"
#include "fmt/core.h"
using namespace Umibozu;

std::string Cartridge::get_title(std::span<const u8> title_bytes) const {
  std::stringstream ss;

  for (auto& title_byte : title_bytes) {
    if (title_byte == 0 || title_byte > 127) {
      break;
    }
    ss << title_byte;
  }

  return ss.str();
}

void Cartridge::print_cart_info() {
  fmt::println("[CART] title: {}", info.title.empty() ? "UNKNOWN" : info.title);
  fmt::println("[CART] mapper string: {}", info.mapper_string);
  fmt::println("[CART] rom banks: {:d}", info.rom_banks);
  fmt::println("[CART] ram banks: {:d}", info.ram_banks);
  fmt::println("[CART] region: {}",
               info.destination_code ? "japan" : "overseas");
  fmt::println("[CART] mem vec size: {}", memory.size());
}

void Cartridge::set_cart_info() {
  u8 mapper_id  = this->memory[0x147];
  u16 rom_banks = 2 * (1 << memory[0x148]);
  u8 ram_banks = 0;
  
  switch (memory[0x149]) {
    case 2: {
      ram_banks = 1;
      break;
    }
    case 3: {
      ram_banks = 4;
      break;
    }
    case 4: {
      ram_banks = 16;
      break;
    }
    case 5: {
      ram_banks = 8;
      break;
    }
    default: {
      break;
    }
  }

  u8 destination_code      = memory[0x14A];

  fmt::println("mapper id: {:#4x}", mapper_id);
  info.title        = get_title(std::span<u8>(memory.begin() + 0x134, 16));
  info.mapper_string    = cart_types.at(mapper_id);
  info.mapper_id        = mapper_id;
  info.rom_banks        = rom_banks;
  info.ram_banks        = ram_banks;
  info.destination_code = destination_code;
}

u8 Cartridge::read8(const u64 address) { return memory[address]; }