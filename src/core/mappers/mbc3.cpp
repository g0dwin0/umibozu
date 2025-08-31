#include "mapper.hpp"
class MBC3 : public Mapper {
 public:
  MBC3() { rom_bank = 1; }
  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart->read8((0x4000 * rom_bank) + (address - 0x4000));
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_bank >= 8) {
        return 0xFF;
      }

      if (register_mode == WRITING_MODE::RAM && rtc_ext_ram_enabled) {
        if (ram_bank == 0) {
          return bus->cart->ext_ram.at(address - 0xA000);
        } else {
          return bus->cart->ext_ram.at(((ram_bank * 0x2000) + (address - 0xA000)) % (bus->cart->info.ram_banks * 0x2000));
        }
      }

      // if (register_mode == WRITING_MODE::RTC && rtc_ext_ram_enabled) {
      //   if (!latched_occured) return 0xFF;
      //   return latched.read_from_active_reg(active_rtc_register);
      // }

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
        // rtc_internal_clock = 0;
        // fmt::println("[MBC3] EXT RAM/RTC DISABLED");
      }

      return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
      if (value == 0) {
        rom_bank = 1;
        return;
      }

      rom_bank = value & (bus->cart->info.rom_banks - 1);
      // fmt::println("new rom bank: {:d}", rom_bank);

      return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
      if (value <= 0x7) {
        register_mode = WRITING_MODE::RAM;
        ram_bank      = value;
      }
      if (value >= 0x08 && value <= 0x0C) {
        register_mode = WRITING_MODE::RTC;
        // active_rtc_register = (RTC_REGISTER)value;
      }

      // if (value >= 0x08 && value <= 0x0C) {
      //   ram_bank = value;
      // }

      // fmt::println("new ram bank: {:d}", ram_bank);
      return;
    }

    if (address >= 0x6000 && address <= 0x7FFF) {
      // if (value == 0x01 && last_rtc_value == 0x00 && rtc_ext_ram_enabled) {
      //   // latched_occured = true;
      //   // latched         = actual;
      //   // fmt::println("latching");
      // } else {
      //   // last_rtc_value = value;
      // }
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      if (register_mode == WRITING_MODE::RAM && rtc_ext_ram_enabled) {
        // fmt::println("in ram write");
        if (ram_bank >= 8) {
          assert(0);
          // fmt::println("more than 8");
          return;
        }

        // fmt::println("ram bank: {}", ram_bank);
        // fmt::println("cart max: {}", bus->cart->info.ram_banks);

        if (ram_bank == 0 || bus->cart->info.ram_banks == 0) {
          bus->cart->ext_ram.at(address - 0xA000) = value;
        } else {
          bus->cart->ext_ram.at(((0x2000 * ram_bank) + (address - 0xA000)) % (0x2000 * bus->cart->info.ram_banks)) = value;
        }
      }
      if (register_mode == WRITING_MODE::RTC && rtc_ext_ram_enabled) {
        // fmt::println("in RTC MODE");
        // fmt::println("writing to RTC register: {:#08x}", (u8)active_rtc_register);

        // rtc_latched.write_to_active_reg(active_rtc_register, value, this->rtc_internal_clock);
        // rtc_actual.write_to_active_reg(active_rtc_register, value, this->rtc_internal_clock);
      }
      return;
    }
    
    bus->write8(address, value);
    return;
  }
};