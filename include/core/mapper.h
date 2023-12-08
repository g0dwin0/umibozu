#pragma once
#include "bus.h"
#include "cart_constants.hpp"
#include "common.h"
#pragma GCC diagnostic ignored "-Wtype-limits"
// TODO: implement MBC1M (>1mb carts)
#define SERIAL_PORT_BUFFER_SIZE 2048

class Mapper {
 public:
  static inline Bus* bus                                 = nullptr;
  u8 banking_mode                                        = 0;
  u8 rom_bank                                            = 0;
  u8 ram_bank                                            = 0;
  bool ram_enabled                                       = false;
  virtual u8 read8(const u16 address)                    = 0;
  virtual void write8(const u16 address, const u8 value) = 0;

  u8 handle_system_memory_read(const u16 address);
  void handle_system_memory_write(const u16 address, const u8 value);
};

Mapper* get_mapper_by_id(u8 mapper_id);