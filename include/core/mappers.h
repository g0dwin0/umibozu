#pragma once
#include <stdexcept>

#include "bus.h"
#include "cart_constants.hpp"
#include "common.h"
#pragma GCC diagnostic ignored "-Wtype-limits"
// TODO: implement MBC1M (>1mb carts)
#define SERIAL_PORT_BUFFER_SIZE 2048

class Mapper {
 public:
  static inline Bus* bus;
  u16 rom_bank;
  u8 ram_bank;
  bool ram_enabled                                       = false;
  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;

  u8 handle_system_memory_read(const u16 address) {
    if (address <= 0x3FFF) {
      return bus->cart.read8(address);
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
      return bus->vram.read8(address);
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
      return bus->wram.read8(address - 0xC000);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->wram.read8((address & 0xDFFF));
    }
    if (address >= 0xFE00 && address <= 0xFE9F) {
      return bus->wram.read8(address);
    }
    if (address >= 0xFEA0 && address <= 0xFEFF) {
      return 0x0;
    }
    if (address >= 0xFF00 && address <= 0xFFFF) {
      return bus->wram.read8(address);
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  };
  void handle_system_memory_write(const u16 address, const u8 value) {
    if (address <= 0x7FFF) {
      throw std::runtime_error("cannot write to ROM area");
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
      return bus->vram.write8(address - 0x8000, value);
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
      return bus->wram.write8(address - 0xC000, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->wram.write8((address & 0xDFFF), value);
    }
    if (address >= 0xFE00 && address <= 0xFE9F) {  // oam
      return bus->wram.write8(address, value);
    }
    if (address >= 0xFEA0 && address <= 0xFEFF) {
      throw std::runtime_error("cannot write to prohibited area");
    }
    // if (address >= 0xFF00 && address <= 0xFF7F) {
    //   return handle_system_io_write(address, value);
    // }
    if (address >= 0xFF80 && address <= 0xFFFF) {  // oam
      return bus->wram.write8(address, value);
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
  }
};

class MBC_1 : public Mapper {
  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * rom_bank) + address);
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      fmt::println("rom bank: {:d}", rom_bank);
      address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;
      return bus->wram.write8(address, value);
    }
    return handle_system_memory_write(address, value);
  }
};
class MBC_1_RAM : public Mapper {
  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * rom_bank) + address);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_enabled) {
        return bus->cart.ext_ram.read8((0x2000 * ram_bank) +
                                       (address % 0xA000));
      }
      return 0xFF;
    }

    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x1FFF) {
      ram_enabled = ((value & 0xF) == 0xA);
      return;
    }
    if (address <= 0x7FFF) {
      fmt::println("rom bank: {:d}", rom_bank);
      address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;

      return bus->wram.write8(address, value);
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
      return bus->vram.write8(address - 0x8000, value);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_enabled) {
        bus->cart.ext_ram.write8((0x2000 * ram_bank) + (address % 0xA000),
                                 value);
      }
      return;
    }

    return handle_system_memory_write(address, value);
  }
};

class ROM_ONLY : public Mapper {
  u8 read8(const u16 address) override {
    if (address <= 0x7FFF) {
      return bus->cart.read8(address);
    }

    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    return handle_system_memory_write(address, value);
  }
};

inline Mapper* get_mapper_by_id(u8 mapper_id) {
  fmt::println("MAPPER ID: {:#04x} ({})", mapper_id, cart_types.at(mapper_id));
  Mapper* mapper = nullptr;
  switch (mapper_id) {
    case 0x0: {
      mapper = dynamic_cast<Mapper*>(new ROM_ONLY());
      break;
    }
    case 0x1: {
      mapper = dynamic_cast<Mapper*>(new MBC_1());
      break;
    }
    case 0x2: {
      mapper = dynamic_cast<Mapper*>(new MBC_1_RAM());
      break;
    }
    default: {
      throw std::runtime_error(
          fmt::format("[MAPPER] unimplemented mapper with ID: {:d} ({})",
                      mapper_id, cart_types.at(mapper_id)));
      break;
    }
  }
  return mapper;
}