#include "double_buffer.hpp"
#include <cassert>

void DoubleBuffer::write(size_t idx, u16 value) {
  assert(idx < (256*256));
  write_buf[idx] = value;
}

void DoubleBuffer::swap_buffers() {
  std::swap(write_buf, disp_buf);
};