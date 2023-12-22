#include "cart.h"

#include <span>
#include <sstream>
#include <vector>

#include "cart_constants.hpp"
#include "common.h"
#include "fmt/core.h"
using namespace Umibozu;

Cartridge::Cartridge()  = default;
Cartridge::~Cartridge() = default;

std::string Cartridge::get_title(std::span<const u8> title_bytes) {
  std::stringstream ss;

  for (auto& title_byte : title_bytes) {
    if (title_byte == 0 || title_byte > 127)
      break;
    ss << title_byte;
  }

  return ss.str();
}
std::string Cartridge::get_manufacturer(u8 index,
                                        std::span<const u8> new_vendor_bytes) {
  std::stringstream ss;
  ss << new_vendor_bytes[0] << new_vendor_bytes[1];
  fmt::println("{}", index);
  fmt::println("{}", atoi(ss.str().c_str()));
  return index == 0x33 ? NEW_MANUFACTURER_MAP.at(atoi(ss.str().c_str()))
                       : OLD_MANUFACTURER_MAP.at(index);
}

void Cartridge::print_cart_info() {
  fmt::println("title: {}", info.title);
  fmt::println("manufacturer: {}", info.manufacturer);
  fmt::println("mapper string: {}", info.mapper_string);
  fmt::println("rom banks: {:d}", info.rom_banks);
  fmt::println("ram banks: {:d}", info.ram_banks);
  fmt::println("destination: {}",
               info.destination_code ? "japanese" : "overseas");
  // fmt::println("cgb support: {}", info.supports_cgb_enhancements);
  fmt::println("mem vec size: {}", memory.size());
}

void Cartridge::set_cart_info() {
  u8 mapper_id  = this->memory[0x147];
  u16 rom_banks = 2 * (1 << memory[0x148]);
  fmt::println("ROM banks: {}", rom_banks);
  u16 ram_banks = 0;
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
      fmt::println("[CART] could not determine ram bank amount");
    }
  }

  u8 destination_code      = memory[0x14A];
  u8 old_manufacturer_code = memory[0x14B];

  info.title        = get_title(std::span<u8>(memory.begin() + 0x134, 16));
  info.manufacturer = get_manufacturer(
      old_manufacturer_code, std::span<u8>(memory.begin() + 0x144, 2));
  info.mapper_string    = cart_types.at(mapper_id);
  info.mapper_id        = mapper_id;
  info.rom_banks        = rom_banks;
  info.ram_banks        = ram_banks;
  info.destination_code = destination_code;
}

u8 Cartridge::read8(const u64 address) { return memory[address]; }