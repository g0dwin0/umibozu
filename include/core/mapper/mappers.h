#pragma once
#include <stdexcept>

#include "bus.h"
#include "cart/cart_constants.hpp"
#include "common.h"
#pragma GCC diagnostic ignored "-Wtype-limits"
// TODO: implement MBC1M (>1mb carts)
#define SERIAL_PORT_BUFFER_SIZE 1
class Mapper {
 public:
  static inline Bus* bus;
  u8 rom_bank;
  u8 ram_bank;
  bool ram_enabled                                       = false;
  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;
};

class MBC_1 : public Mapper {
  u8 read8(const u16 address) override {
    if (address == 0xFF44) {
      return 0x90;
    }
    if (address <= 0x3FFF) {
      return bus->cart.read8(address);
    }
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * rom_bank) + address);
    }
    if (address >= 0x8000 && address <= 0xDFFF) {
      return bus->ram.read8(address);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.read8((address & 0xDFFF));
    }
    if (address >= 0xFE00 && address <= 0xFFFF) {
      return bus->ram.read8(address);
    }
    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x7FFF) {
      fmt::println("rom bank: {:d}", rom_bank);
      address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDFFF), value);
    }
    if (address == SC && value == 0x81 && bus->ram.data[SC] & 0x80) {
      bus->serial_port_buffer[bus->serial_port_index++] = bus->ram.data[SB];
      std::string str_data(bus->serial_port_buffer, SERIAL_PORT_BUFFER_SIZE);
      fmt::println("serial data: {}", str_data);
    }
    return bus->ram.write8(address, value);
    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
  }
};
class MBC_1_RAM : public Mapper {
  u8 read8(const u16 address) override {
    if (address == 0xFF44) {
      return 0x90;
    }
    if (address <= 0x3FFF) {
      return bus->cart.read8(address);
    }
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
    if (address >= 0x8000 && address <= 0xDFFF) {
      return bus->ram.read8(address);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.read8((address & 0xDFFF));
    }
    if (address >= 0xFE00 && address <= 0xFFFF) {
      return bus->ram.read8(address);
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x1FFF) {
      ram_enabled = ((value & 0xF) == 0xA);
      return;
    }
    if (address <= 0x7FFF) {
      fmt::println("rom bank: {:d}", rom_bank);
      address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;

      return bus->ram.write8(address, value);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_enabled) {
        return bus->cart.ext_ram.write8(
            (0x2000 * ram_bank) + (address % 0xA000), value);
      }
      return;
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDFFF), value);
    }
    if (address == SC && value == 0x81 && bus->ram.data[SC] & 0x80) {
      bus->serial_port_buffer[bus->serial_port_index++] = bus->ram.data[SB];
      std::string str_data(bus->serial_port_buffer, SERIAL_PORT_BUFFER_SIZE);
      fmt::println("serial data: {}", str_data);
    }
    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
  }
};

class ROM_ONLY : public Mapper {
  u8 read8(const u16 address) override {
    if (address <= 0x7FFF) {
      return bus->cart.read8(address);
    }
    if (address >= 0x8000 && address <= 0x9FFF) {
      // read vram
      throw std::runtime_error("mapper does not support vram reading");
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      return bus->cart.ext_ram.read8(address % 0xA000);
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
      return bus->ram.read8(address);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.read8((address & 0xDFFF));
    }
    if (address >= 0xFE00 && address <= 0xFFFF) {
      return bus->ram.read8(address);
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  }
  void write8(const u16 address, const u8 value) override {
    if (address >= 0xA000 && address <= 0xBFFF) {
      return bus->cart.ext_ram.write8((address % 0xA000), value);
    }
    if (address >= 0xC000 && address <= 0xDFFF) {
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDFFF), value);
    }
    if (address >= 0xFE00 && address <= 0xFFFF) {
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDFFF), value);
    }
    if (address == SC && value == 0x81 && bus->ram.data[SC] & 0x80) {
      bus->serial_port_buffer[bus->serial_port_index++] = bus->ram.data[SB];
      std::string str_data(bus->serial_port_buffer, SERIAL_PORT_BUFFER_SIZE);
      fmt::println("serial data: {}", str_data);
    }
    return bus->ram.write8(address, value);
    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
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