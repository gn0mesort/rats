#ifndef RATS_GEOMETRY_HPP
#define RATS_GEOMETRY_HPP

#include <cinttypes>

namespace rats {

struct offset_2d final {
  std::int16_t x;
  std::int16_t y;
};

struct offset_3d final {
  std::int16_t x;
  std::int16_t y;
  std::int16_t z;
};

struct extent_2d final {
  std::uint16_t width;
  std::uint16_t height;
};

struct extent_3d final {
  std::uint16_t width;
  std::uint16_t height;
  std::uint16_t depth;
};

struct rect_2d final {
  offset_2d offset;
  extent_2d extent;
};

bool operator==(const offset_2d& lhs, const offset_2d& rhs);
bool operator!=(const offset_2d& lhs, const offset_2d& rhs);

}

#endif
