#ifndef RATS_GRAPHICS_HPP
#define RATS_GRAPHICS_HPP

#include "math.hpp"

namespace rats {

struct rgba final {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

rgba denormalized(const vec4& color);
vec4 normalized(const rgba& color);
vec4 premultiply_alpha(const vec4& color);
vec4 to_srgb(const vec4& color);
vec4 to_linear(const vec4& color);
vec4 blend(const vec4& a, const vec4& b, const bool premultiplied = false);

}

#endif
