#include "mapper.hpp"
class MBC1 : public Mapper {
  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * (rom_bank == 0 ? 1 : rom_bank)) +
                             address - 0x4000);
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ext_ram_enabled) {
        if (banking_mode == 0) {
          return bus->cart.ext_ram.at((0x2000 * 0) + (address - 0xA000));
        } else {
          return bus->cart.ext_ram.at((0x2000 * ram_bank) +
                                         (address - 0xA000));
        }
      }
      return 0xFF;
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x1FFF) {
      if ((value & 0xF) == 0xA) {
        ext_ram_enabled = true;
      } else {
        ext_ram_enabled = false;
      }
      return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
      rom_bank = (value & (bus->cart.info.rom_banks - 1));
      // rom_bank &= bus->cart.info.rom_banks;
      return;
    }
    if (address >= 0x4000 && address <= 0x5FFF) {
      if (bus->cart.info.ram_banks >= 4) { ram_bank = value & 0x3; }
      return;
    }
    if (address >= 0x6000 && address <= 0x7FFF) {
      banking_mode = value & 0x1;
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ext_ram_enabled) {
        assert(ram_bank <= 3);
        if (banking_mode == 0) {
          bus->cart.ext_ram.at((0x2000 * 0) + (address - 0xA000)) = value;
        } else {
          bus->cart.ext_ram.at((0x2000 * ram_bank) + (address - 0xA000))= value;
        }
      }
      return;
    }
    return handle_system_memory_write(address, value);
  }
};