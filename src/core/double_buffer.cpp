#include "double_buffer.hpp"
#include <cassert>

void DoubleBuffer::write(size_t idx, u16 value) {
  assert(idx < (256*256));
  write_buf[idx] = value;

  // if (idx == (241 * 160) - 1) {
  //   request_swap();
  // }
}

void DoubleBuffer::request_swap() {
  swap_buffers();
};

void DoubleBuffer::swap_buffers() {
  std::swap(write_buf, disp_buf);
};