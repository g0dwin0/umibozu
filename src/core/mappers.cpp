#include <stdexcept>
#include "cart_constants.hpp"
#include "core/mapper.hpp"
#include "mappers/mbc1.cpp"
#include "mappers/mbc3.cpp"
#include "mappers/mbc5.cpp"

u8 Mapper::handle_system_memory_read(const u16 address) {
  if (address <= 0x3FFF) {
    return bus->cart.read8(address);
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    // fmt::println("mapper vram: {:#04x}", address);
    return bus->vram->at((address - 0x8000));
  }
  if (address >= 0xC000 && address <= 0xCFFF) {
    return bus->wram_banks[0].at(address - 0xC000);
  }
  if (address >= 0xD000 && address <= 0xDFFF) {
    return bus->wram->at((address - 0xD000));
  }

  if (address >= 0xE000 && address <= 0xFDFF) {
    return handle_system_memory_read((address & 0xDFFF));
  }
  if (address >= 0xFE00 && address <= 0xFE9F) {
    return bus->oam.at(address - 0xFE00);
  }
  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return 0x00;
  }
  if (address >= 0xFF00 && address <= 0xFF7F) {
    return bus->io.at(address - 0xFF00);
  }
  if (address >= 0xFF80 && address <= 0xFFFE) {
    return bus->hram.at(address - 0xFF80);
  }
  if (address == 0xFFFF) {
    return bus->io.at(IE);
  }

  throw std::runtime_error(fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
}

void Mapper::handle_system_memory_write(const u16 address, const u8 value) {
  if (address <= 0x7FFF) {
    return;
  }
  if (address >= 0x8000 && address <= 0x9FFF) {
    // if (bus->vbk == 1) {
    //   fmt::println("writing to VRAM {:d}:{:#04x}", +bus->vbk, address);
    // }
    bus->vram->at(address - 0x8000) = value;
    return;
  }
  if (address >= 0xC000 && address <= 0xCFFF) {
    bus->wram_banks[0].at(address - 0xC000) = value;
    return;
  }
  if (address >= 0xD000 && address <= 0xDFFF) {
    bus->wram->at(address - 0xD000) = value;
    return;
  }

  if (address >= 0xE000 && address <= 0xFDFF) {
    return handle_system_memory_write((address & 0xDFFF), value);
  }
  if (address >= 0xFE00 && address <= 0xFE9F) {  // oam
    bus->oam.at(address - 0xFE00) = value;
    return;
  }
  if (address >= 0xFEA0 && address <= 0xFEFF) {  // unused/illegal
    return;
  }
  if (address >= 0xFF80 && address <= 0xFFFE) {
    bus->hram.at(address - 0xFF80) = value;
    return;
  }

  throw std::runtime_error(fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
}

class ROM_ONLY : public Mapper {
 public:
  ROM_ONLY() {
    if (bus->cart.info.ram_banks > 0) {
      ext_ram_enabled = true;
    }
  }

 private:
  u8 read8(const u16 address) override {
    if (address <= 0x7FFF) {
      return bus->cart.read8(address);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (!ext_ram_enabled) {
        return 0xFF;
      }
      fmt::println("[MAPPER] reading from ext ram");
      return bus->cart.ext_ram.at(address - 0xA000);
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (!ext_ram_enabled) {
        return;
      }
      fmt::println("[MAPPER] writing to ext ram");
      bus->cart.ext_ram.at(address - 0xA000) = value;
      return;
    }
    return handle_system_memory_write(address, value);
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
    case 0x10:
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