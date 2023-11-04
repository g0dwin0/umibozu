#pragma once
#include <string>
#include <unordered_map>

#include "cart_constants.hpp"
#include "common.h"
namespace Umibozu {
  class Cartridge {
    struct Info {
      std::string title;
      std::string manufacturer;
      std::string mapper_string;
      u8 mapper_id;
      u32 rom_size;
      u32 ram_size;
      u8 destination_code;
      bool supports_cgb_enhancements;
    };

   private:
    bool logo_is_valid(std::vector<u8>&);
    std::string get_manufacturer(u8, std::vector<u8>);
    std::string get_mapper_string(u8);
    void print_cart_info();

   public:
    void set_cart_info();
    std::vector<u8> memory;
    Info info;
    Cartridge();
    ~Cartridge();

    u8 read8(const u16);
    void write8(const u16, const u8);
  };
}  // namespace Umibozu
