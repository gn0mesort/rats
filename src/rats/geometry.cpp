#include "rats/geometry.hpp"

namespace rats {

  bool operator==(const offset_2d& lhs, const offset_2d& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }

  bool operator!=(const offset_2d& lhs, const offset_2d& rhs) {
    return !(lhs == rhs);
  }

}
