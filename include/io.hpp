#pragma once
#include <fstream>
#include <iterator>
#include <vector>
#include "common.hpp"

struct File {
  std::vector<u8> data;
  std::string path;
};

inline File read_file(std::string filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file.good()) {
    assert(0);
  }

  file.unsetf(std::ios::skipws);
  std::streampos fileSize;
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<u8> vec;
  vec.reserve(fileSize);

  vec.insert(vec.begin(), std::istream_iterator<u8>(file), std::istream_iterator<u8>());

  return File{vec, filename};
}