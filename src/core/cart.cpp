#include "cart.h"
#include "io.hpp"
#include "common.h"
#include "mappers.h"
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
  fmt::println("rom size: {:d} KiB", info.rom_size);
  fmt::println("ram size: {:d} KiB", info.ram_size);
  fmt::println("destination: {}",
               info.destination_code ? "japanese" : "overseas");
  fmt::println("cgb support: {}", info.supports_cgb_enhancements);
}
std::string Cartridge::get_mapper_string(u8 cartridge_type) {
  return cart_types.at(cartridge_type);
};
// void Cartridge::load_cart(std::vector<u8> data) {
//   this->memory = data;
//   (void)(data);

// };
void Cartridge::set_cart_info() {
  bool cgb_enhancements = memory[0x143] == 0x80;

  u8 mapper_id        = this->memory[0x147];
  u32 rom_size             = 32 * (1 << memory[0x148]);
  u32 ram_size             = 32 * (1 << memory[0x149]);
  u8 destination_code      = memory[0x14A];
  u8 old_manufacturer_code = memory[0x14B];

  info.manufacturer = get_manufacturer(
      old_manufacturer_code, get_bytes_in_range(memory, 0x144, 0x145));
  info.mapper_string             = get_mapper_string(mapper_id);
  info.mapper_id                 = mapper_id;
  info.rom_size                  = rom_size;
  info.ram_size                  = ram_size;
  info.destination_code          = destination_code;
  info.supports_cgb_enhancements = cgb_enhancements;
};

u8 Cartridge::read8(const u16 address) {
  // fmt::println("address: {:#04x}", address);
  // fmt::println("size of data: {:#04x}", memory.size());
  // fmt::println("size of data: {:#04x}", memory.size());
  
  if (address >= 0xA000 && address <= 0xBFFF) {
    fmt::println("ram bank!");
    exit(-1);
  }
  return memory.at(address);
}
void Cartridge::write8(const u16 address, const u8 value) {
  if (address >= 0xA000 && address <= 0xBFFF) {
    fmt::println("ram bank!");
    exit(-1);
  }
  memory.at(address) = value;
}
