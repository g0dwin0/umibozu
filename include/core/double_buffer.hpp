#pragma once

#include "common.hpp"

struct DoubleBuffer {
    u16* write_buf = nullptr;
    u16* disp_buf  = nullptr;

   public:
    DoubleBuffer() = delete;
    DoubleBuffer(u16* _write_buf, u16* _disp_buf) : write_buf(_write_buf), disp_buf(_disp_buf) {}

    void write(size_t idx, u16 value);
    void request_swap();
    void swap_buffers();
  };