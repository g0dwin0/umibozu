#include <stdexcept>

#include "cart_constants.hpp"
#include "core/mapper.hpp"
#include "mappers/mbc1.cpp"
#include "mappers/mbc3.cpp"
#include "mappers/mbc5.cpp"

class ROM_ONLY : public Mapper {
 public:
  ROM_ONLY() {
    if (bus->cart->info.ram_banks > 0) {
      rtc_ext_ram_enabled = true;
    }
  }

 private:
  u8 read8(const u16 address) override {
    if (address <= 0x7FFF) {
      return bus->cart->read8(address);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (!rtc_ext_ram_enabled) {
        return 0xFF;
      }
      fmt::println("[MAPPER] reading from ext ram");
      return bus->cart->ext_ram.at(address - 0xA000);
    }
    return bus->read8(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (!rtc_ext_ram_enabled) {
        return;
      }
      fmt::println("[MAPPER] writing to ext ram");
      bus->cart->ext_ram.at(address - 0xA000) = value;
      return;
    }
    bus->write8(address, value);
    return;
  }
};

Mapper* get_mapper_by_id(u8 mapper_id) {
  fmt::println("[MAPPER] MAPPER ID: {:#04x} ({})", mapper_id, cart_types.at(mapper_id));

  Mapper* mapper;

  switch (mapper_id) {
    case 0x0:
    case 0x8: {
      mapper = new ROM_ONLY();
      break;
    }
    case 0x1:
    case 0x2:
    case 0x3: {
      mapper = new MBC1();
      break;
    }
    case 0xF:
    case 0x10:  // MBC30
    case 0x11:
    case 0x12:
    case 0x13: {
      mapper = new MBC3();
      break;
    }
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E: {
      mapper = new MBC5();
      break;
    }

    default: {
      throw std::runtime_error(fmt::format("[MAPPER] unimplemented mapper with ID: {:d} ({})", mapper_id, cart_types.at(mapper_id)));
    }
  }
  mapper->id = mapper_id;
  return mapper;
}