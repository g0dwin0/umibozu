#include <stdexcept>

#include "cart_constants.hpp"
#include "core/mapper.hpp"
#include "mappers/rom_only.cpp"
#include "mappers/mbc1.cpp"
#include "mappers/mbc3.cpp"
#include "mappers/mbc5.cpp"

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