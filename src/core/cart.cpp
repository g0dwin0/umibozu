#include "cart.h"

#include "common.h"
#include "io.hpp"
#include "mapper.h"
#define LOGO_START 0x104
using namespace Umibozu;

Cartridge::Cartridge(){};
Cartridge::~Cartridge(){};

std::string Cartridge::get_manufacturer(u8 index,
                                        std::vector<u8> new_vendor_bytes) {
  std::stringstream ss;
  ss << new_vendor_bytes[0] << new_vendor_bytes[1];

  return index == 0x33 ? NEW_MANUFACTURER_MAP.at(atoi(ss.str().c_str()))
                       : OLD_MANUFACTURER_MAP.at(index);
}

void Cartridge::print_cart_info() {
  fmt::println("manufacturer: {}", info.manufacturer);
  fmt::println("mapper string: {}", info.mapper_string);
  fmt::println("rom banks: {:d}", info.rom_banks);
  fmt::println("ram banks: {:d}", info.ram_banks);
  fmt::println("destination: {}",
               info.destination_code ? "japanese" : "overseas");
  fmt::println("cgb support: {}", info.supports_cgb_enhancements);
}
std::string Cartridge::get_mapper_string(u8 cartridge_type) {
  return cart_types.at(cartridge_type);
};

void Cartridge::set_cart_info() {
  bool cgb_enhancements = memory[0x143] == 0x80;

  u8 mapper_id  = this->memory.at(0x147);
  u64 rom_banks = 2 * (1 << memory[0x148]);
  u64 ram_banks = 0;
  switch (memory[0x149]) {
    case 0: {
      ram_banks = 0;
      break;
    }

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
      // fmt::println("couldn't determine ram banks");
      // assert(1==2);
      break;
    }
  }
  u8 destination_code      = memory[0x14A];
  u8 old_manufacturer_code = memory[0x14B];

  info.manufacturer = get_manufacturer(
      old_manufacturer_code, get_bytes_in_range(memory, 0x144, 0x145));
  info.mapper_string             = get_mapper_string(mapper_id);
  info.mapper_id                 = mapper_id;
  info.rom_banks                 = rom_banks;
  info.ram_banks                 = ram_banks;
  info.destination_code          = destination_code;
  info.supports_cgb_enhancements = cgb_enhancements;
};

u8 Cartridge::read8(const u16 address) { return memory.at(address); }
// void Cartridge::write8(const u16 address, const u8 value) {
//   memory.at(address) = value;
// }
