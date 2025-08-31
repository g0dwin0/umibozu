#include "mapper.hpp"
class MBC5 : public Mapper {
 public:
  MBC5() { rom_bank = 1; }

 private:
  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart->read8((0x4000 * rom_bank) + (address - 0x4000));
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (rtc_ext_ram_enabled) {
        if (ram_bank == 1) {
          return bus->cart->ext_ram.at(address - 0xA000);
        } else {
          return bus->cart->ext_ram.at(((ram_bank * 0x2000) + (address - 0xA000)) % (bus->cart->info.ram_banks * 0x2000));
        }
      }
      return 0xFF;
    }

    return bus->read8(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x1FFF) {
      if ((value & 0xF) == 0xA) {
        rtc_ext_ram_enabled = true;
      } else {
        rtc_ext_ram_enabled = false;
      }
      return;
    }

    if (address >= 0x2000 && address <= 0x2FFF) {
      rom_bank = value & (bus->cart->info.rom_banks - 1);
      return;
    }
    if (address >= 0x3000 && address <= 0x3FFF) {
      if (value & 0x1) {
        rom_bank |= (1 << 8);
      } else {
        rom_bank &= ~(1 << 8);
      }

      return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
      if (value < 0xF) {
        ram_bank = value;
      }
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (rtc_ext_ram_enabled) {
        if (ram_bank == 1) {
          bus->cart->ext_ram.at(((0x2000 * 0) + (address - 0xA000))) = value;
        } else {
          bus->cart->ext_ram.at(((0x2000 * ram_bank) + (address - 0xA000)) % (0x2000 * bus->cart->info.ram_banks)) = value;
        }
      }
      return;
    }

    bus->write8(address, value);
    return;
  }
};