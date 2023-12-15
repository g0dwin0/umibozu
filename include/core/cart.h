#pragma once
#include <span>
#include <string>

#include "common.h"
namespace Umibozu {
  class Cartridge {
   private:
    std::string get_title(std::span<const u8>);
    std::string get_manufacturer(u8, std::span<const u8>);
    struct Info {
      std::string title;
      std::string manufacturer;
      std::string mapper_string;
      u8 mapper_id;
      u16 rom_banks;
      u16 ram_banks;
      u8 destination_code;
      bool supports_cgb_enhancements;
    };

   public:
    void print_cart_info();
    void load_cart(std::vector<u8>& data);
    void set_cart_info();
    std::vector<u8> memory;
    RAM ext_ram = RAM(0x10000);
    Info info;
    Cartridge();
    ~Cartridge();

    u8 read8(const u64);
  };
}  // namespace Umibozu
