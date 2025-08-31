#include "mapper.hpp"
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


