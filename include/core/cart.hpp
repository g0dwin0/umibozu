#pragma once
#include <span>
#include <string>
#include <vector>
#include <array>
#include "common.hpp"
namespace Umibozu {
  class Cartridge {
   private:
    std::string get_title(std::span<const u8>) const;
    
    struct Info {
      std::string title;
      std::string mapper_string;
      std::string path;
      u8 mapper_id;
      u16 rom_banks;
      u16 ram_banks;
      u8 destination_code;
    };

   public:
    void print_cart_info();
    void set_cart_info();
    std::vector<u8> memory;
    std::array<u8, 0x100000> ext_ram;
    Info info;

    u8 read8(u64);
  };
} 
