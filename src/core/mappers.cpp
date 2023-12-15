#include "cart_constants.hpp"
#include "core/mapper.h"

u8 Mapper::handle_system_memory_read(const u16 address) {
  if (address <= 0x3FFF) {
    return bus->cart.read8(address);
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    return bus->vram.read8(address - 0x8000);
  }
  if (address >= 0xC000 && address <= 0xDFFF) {
    return bus->wram.read8(address - 0xC000);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->wram.read8((address & 0xDFFF));
  }
  if (address >= 0xFE00 && address <= 0xFE9F) {
    return bus->oam.read8(address - 0xFE00);
  }
  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return 0xFF;
  }
  if (address >= 0xFF00 && address <= 0xFFFF) {
    return bus->wram.read8(address);
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}

void Mapper::handle_system_memory_write(const u16 address, const u8 value) {
  if (address <= 0x7FFF) {
    return;
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    // fmt::println("wrote to vram");
    return bus->vram.write8(address - 0x8000, value);
  }
  if (address >= 0xC000 && address <= 0xDFFF) {
    return bus->wram.write8(address - 0xC000, value);
  }
  if (address >= 0xE000 && address <= 0xFDFF) {
    return bus->wram.write8((address & 0xDFFF), value);
  }
  if (address >= 0xFE00 && address <= 0xFE9F) {  // oam
    return bus->oam.write8(address - 0xFE00, value);
  }
  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return;
  }
  if (address >= 0xFF00 && address <= 0xFFFF) {
    return bus->wram.write8(address, value);
  }

  throw std::runtime_error(
      fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
};

class ROM_ONLY : public Mapper {
  u8 read8(const u16 address) override {
    if (address <= 0x7FFF) {
      return bus->cart.read8(address);
    }

    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address >= 0xA000 && address <= 0xBFFF) {
      return bus->cart.ext_ram.write8(address - 0xA000, value);
    }
    return handle_system_memory_write(address, value);
  }
};

class MBC1 : public Mapper {
  u8 read8(const u16 address) {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * (rom_bank == 0 ? 1 : rom_bank)) +
                             address - 0x4000);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_rtc_enabled) {
        if (banking_mode == 0) {
          return bus->cart.ext_ram.read8((0x2000 * 0) + (address - 0xA000));
        } else {
          return bus->cart.ext_ram.read8((0x2000 * ram_bank) +
                                         (address - 0xA000));
        }
      }
      return 0xFF;
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) {
    if (address <= 0x1FFF) {
      if ((value & 0xF) == 0xA) {
        ram_rtc_enabled = true;
      } else {
        ram_rtc_enabled = false;
      }
      return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
      rom_bank = (value & 0x1f);
      // rom_bank &= bus->cart.info.rom_banks;
      return;
    }
    if (address >= 0x4000 && address <= 0x5FFF) {
      if (bus->cart.info.ram_banks >= 4) {
        ram_bank = value & 0x3;
      }
      return;
    }
    if (address >= 0x6000 && address <= 0x7FFF) {
      banking_mode = value & 0x1;
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_rtc_enabled) {
        assert(ram_bank <= 3);
        if (banking_mode == 0) {
          bus->cart.ext_ram.write8((0x2000 * 0) + (address % 0xA000), value);
        } else {
          bus->cart.ext_ram.write8((0x2000 * ram_bank) + (address % 0xA000),
                                   value);
        }
      }
      return;
    }
    return handle_system_memory_write(address, value);
  }
};
class MBC3 : public Mapper {
  u8 read8(const u16 address) {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * rom_bank) + (address - 0x4000));
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_rtc_enabled) {
        return bus->cart.ext_ram.read8((0x2000 * ram_bank) +
                                       (address - 0xA000));
      }
      return 0xFF;
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) {
    if (address <= 0x1FFF) {
      if ((value & 0xF) == 0xA) {
        ram_rtc_enabled = true;
      } else {
        ram_rtc_enabled = false;
      }
      return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
      if (value == 0) {
        rom_bank = 1;
        return;
      }
      rom_bank = value;
      return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
      if (value >= 0x0 && value <= 0x3) {
        ram_bank = value & 0x3;
      }
      return;
    }

    if (address >= 0x6000 && address <= 0x7FFF) {
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_rtc_enabled) {
        assert(ram_bank <= 3);
        bus->cart.ext_ram.write8((0x2000 * ram_bank) + (address % 0xA000),
                                 value);
      }
      return;
    }
    return handle_system_memory_write(address, value);
  }
};
Mapper* get_mapper_by_id(u8 mapper_id) {
  fmt::println("MAPPER ID: {:#04x} ({})", mapper_id, cart_types.at(mapper_id));
  Mapper* mapper = nullptr;
  switch (mapper_id) {
    case 0x0: {
      mapper = dynamic_cast<Mapper*>(new ROM_ONLY());
      break;
    }
    case 0x1:
    case 0x2:
    case 0x3: {
      mapper = dynamic_cast<Mapper*>(new MBC1());
      break;
    }
    case 0xF:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: {
      mapper = dynamic_cast<Mapper*>(new MBC3());
      break;
    }

    default: {
      throw std::runtime_error(
          fmt::format("[MAPPER] unimplemented mapper with ID: {:d} ({})",
                      mapper_id, cart_types.at(mapper_id)));
    }
  }
  return mapper;
}