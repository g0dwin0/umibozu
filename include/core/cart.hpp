#pragma once
#include <memory>
#include <span>
#include <string>

#include "common.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class Cartridge {
 private:
  static std::string get_title(std::span<const u8>);
  static std::string get_manufacturer(u8, std::span<const u8>);

#ifndef CPU_TEST_MODE
  std::shared_ptr<spdlog::logger> cartridge_logger = spdlog::stdout_color_mt("CART");
#endif

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
  std::vector<u8> data    = std::vector<u8>(0x100000);
  std::vector<u8> ext_ram = std::vector<u8>(0x100000);
  Info info;
};
