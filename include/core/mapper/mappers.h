#pragma once
#include "bus.h"
#include "cart/cart_constants.hpp"
#include "common.h"
#include "fmt/core.h"

// TODO: implement MBC1M (>1mb carts)
class Mapper {
 public:
  static inline Bus* bus;
  u32 rom_bank;
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
      return bus->ram.read8((address & 0xDDFF));
    }
    if (address >= 0xFE00 && address <= 0xFFFE) {
      return bus->ram.read8(address);
    }
    if (address == 0xFFFF) {
      return bus->ram.ram[IE];
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x7FFF) {
      address >= 0x2000 ? (rom_bank = value & 0b00000111) : 0;
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDDFF), value);
    }
    if (address == SC && value == 0x81 && bus->ram.ram[SC] & 0x80) {
      bus->serial_port_buffer[bus->serial_port_index++] = bus->ram.ram[SB];
      std::string str_data(bus->serial_port_buffer, 1024);
      fmt::println("serial data: {}", str_data);
    }
    return bus->ram.write8(address, value);
    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU write: {:#04x}", address));
  }
};

class NO_MBC : public Mapper {
  u8 read8(const u16 address) override {
    if (address == 0xFF44) {
      return 0x90;
    }
    if (address <= 0x3FFF) {
      return (u8)bus->cart.read8(address);
    }
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * bus->cart.rom_bank) + address);
    }
    if (address >= 0x8000 && address <= 0xDFFF) {
      return bus->ram.read8(address);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.read8((address & 0xDDFF));
    }
    if (address >= 0xFE00 && address <= 0xFFFE) {
      return bus->ram.read8(address);
    }
    if (address == 0xFFFF) {
      return bus->ram.ram[IE];
    }

    throw std::runtime_error(
        fmt::format("[CPU] out of bounds CPU read: {:#04x}", address));
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x7FFF) {
      address >= 0x2000 ? (bus->cart.rom_bank = value & 0b00000111) : 0;
      return bus->ram.write8(address, value);
    }
    if (address >= 0xE000 && address <= 0xFDFF) {
      return bus->ram.write8((address & 0xDDFF), value);
    }
    if (address == SC && value == 0x81 && bus->ram.ram[SC] & 0x80) {
      bus->serial_port_buffer[bus->serial_port_index++] = bus->ram.ram[SB];
      std::string str_data(bus->serial_port_buffer, 1024);
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
    case 0: {
      mapper = dynamic_cast<Mapper*>(new NO_MBC());
      break;
    }
    case 1: {
      mapper = dynamic_cast<Mapper*>(new MBC_1());
      break;
    }
    default: {
        throw std::runtime_error(fmt::format("[MAPPER] unimplemented mapper with ID: {:d}", mapper_id));
        break;
    }
  }
  printf("%d\n", mapper_id);

  return mapper;
}