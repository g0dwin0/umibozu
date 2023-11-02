#pragma once
#include <string>
#include <unordered_map>

#include "common.h"
#include "cart_constants.hpp"
namespace Umibozu {
  class Cartridge {
    struct Info {
      std::string title;
      std::string manufacturer;
      std::string mapper_string;
      u32 rom_size;
      u32 ram_size;
      u8 destination_code;
      bool supports_cgb_enhancements;
    };

   private:
    std::vector<u8> memory;
    Info info;

    bool logo_is_valid(std::vector<u8>&);
    Cartridge::Info get_cart_info();
    std::string get_manufacturer(u8, std::vector<u8>);
    std::string get_mapper_string(u8);
    void print_cart_info();

   public:
    Cartridge();
    ~Cartridge();
    void load_cart(std::vector<u8> data);
    u32 rom_bank = 0;

    u8 read8(const u16);
    void write8(const u16, const u8);
  };
}  // namespace Umibozu
