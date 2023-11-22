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
      u8 rom_size;
      u8 ram_size;
      u8 destination_code;
      bool supports_cgb_enhancements;
    };

   private:
    std::string get_manufacturer(u8, std::vector<u8>);
    std::string get_mapper_string(u8);

   public:
    void print_cart_info();
    void load_cart(std::vector<u8> data);
    void set_cart_info();
    std::vector<u8> memory;
    RAM ext_ram = RAM(0x10000);
    Info info;
    Cartridge();
    ~Cartridge();

    u8 read8(const u16);
    void write8(const u16, const u8);
  };
}  // namespace Umibozu
