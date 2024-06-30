#include "mapper.h"
class MBC3 : public Mapper {
 public:
  MBC3() { rom_bank = 1; }

  u8 read8(const u16 address) override {
    if (address >= 0x4000 && address <= 0x7FFF) {
      return bus->cart.read8((0x4000 * rom_bank) + (address - 0x4000));
    }
    if (address >= 0xA000 && address <= 0xBFFF) {
      if (ram_bank >= 8) { return 0xFF; }
      if (register_mode == WRITING_MODE::RAM && ext_ram_enabled) {
        return bus->cart.ext_ram.read8((0x2000 * ram_bank) +
                                       (address - 0xA000));
      }
      if (register_mode == WRITING_MODE::RTC && rtc_enabled) {
        if (!latched_occured) return 0xFF;
        return latched.read_from_active_reg(active_rtc_register);
      }

      return 0xFF;
    }
    return handle_system_memory_read(address);
  }
  void write8(const u16 address, const u8 value) override {
    if (address <= 0x1FFF) {
      if ((value & 0xF) == 0xA) {
        ext_ram_enabled = true;
        rtc_enabled     = true;
        fmt::println("[MBC3] EXT RAM/RTC ENABLED");
      } else {
        ext_ram_enabled = false;
        rtc_enabled     = false;
        // rtc_internal_clock = 0;
        fmt::println("[MBC3] EXT RAM/RTC DISABLED"); 
      }
      return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
      if (value == 0) {
        rom_bank = 1;
        return;
      }

      rom_bank = value & (bus->cart.info.rom_banks - 1);
      // fmt::println("new rom bank: {:d}", rom_bank);

      return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
      if (value >= 0x0 && value <= 0x7) {
        register_mode = WRITING_MODE::RAM;
        ram_bank      = value;
      }
      if (value >= 0x08 && value <= 0x0C) {
        register_mode       = WRITING_MODE::RTC;
        active_rtc_register = (RTC_REGISTERS)value;
      }

      // if (value >= 0x08 && value <= 0x0C) {
      //   ram_bank = value;
      // }

      // fmt::println("new ram bank: {:d}", ram_bank);
      return;
    }

    if (address >= 0x6000 && address <= 0x7FFF) {
      if (value == 0x01 && last_rtc_value == 0x00 && rtc_enabled) {
        latched_occured = true;
        latched         = actual;
        // fmt::println("latching");
      } else {
        last_rtc_value = value;
      }
      return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
      fmt::println("CATCH!");
      // fmt::println("RAM BANK: {:#08x}");

      if (register_mode == WRITING_MODE::RAM && ext_ram_enabled) {
        if (ram_bank >= 8) { return; }
        fmt::println("writing to ext ram loc: {:#08x}",
                     (0x2000 * ram_bank) + (address - 0xA000));
        bus->cart.ext_ram.write8((0x2000 * ram_bank) + (address - 0xA000),
                                 value);
      }
      if (register_mode == WRITING_MODE::RTC && rtc_enabled) {
        fmt::println("writing to RTC register: {:#08x}", (u8)active_rtc_register);

        latched.write_to_active_reg(active_rtc_register, value, rtc_internal_clock);
        actual.write_to_active_reg(active_rtc_register, value, rtc_internal_clock);
      }
      return;
    }
    // fmt::println("address: {:#08x} value: {:#08x}", address, value);
    return handle_system_memory_write(address, value);
  }
};