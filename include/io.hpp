#include <fstream>
#include "common.h"

namespace Umibozu {
  inline std::vector<u8> read_file(std::string filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.good()) {
      throw std::runtime_error(
          fmt::format("io: failed to load file: {}", filename));
    }

    file.unsetf(std::ios::skipws);
    std::streampos fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<u8> vec;
    vec.reserve(fileSize);

    vec.insert(vec.begin(), std::istream_iterator<u8>(file),
               std::istream_iterator<u8>());

    return vec;
  }


  inline std::vector<u8> get_bytes_in_range(std::vector<u8>& vector, u16 start,
                                            u16 end) {
    if (start > end) {
      throw std::runtime_error(
          fmt::format("[IO] {}: invalid range, start: {:d}, end {:d}", __func__,
                      start, end));
    }

    std::vector<u8> vec;

    for (u16 i = 0; i < (end - start) + 1; i++) {
      vec.push_back(vector[start + i]);
    }
    return vec;
  }

}  // namespace Umibozu