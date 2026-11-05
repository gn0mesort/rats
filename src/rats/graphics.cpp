#include "rats/graphics.hpp"


namespace {

float to_srgb(const float c) {
  if (c <= 0.0031308f)
  {
    return 12.92f * c;
  }
  else
  {
    return 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
  }
}

float to_linear(const float c) {
  if (c <= 0.04045f)
  {
    return c / 12.92f;
  }
  else
  {
    return std::pow((c + 0.055f) / 1.055f, 2.4f);
  }
}

}

namespace rats {

rgba denormalized(const vec4& color) {
  auto res = rgba{ };
  res.r = static_cast<unsigned char>(color.x * 255.0f);
  res.g = static_cast<unsigned char>(color.y * 255.0f);
  res.b = static_cast<unsigned char>(color.z * 255.0f);
  res.a = static_cast<unsigned char>(color.w * 255.0f);
  return res;
}

vec4 normalized(const rgba& color) {
  auto res = vec4{ };
  res.x = static_cast<float>(color.r) / 255.0f;
  res.y = static_cast<float>(color.g) / 255.0f;
  res.z = static_cast<float>(color.b) / 255.0f;
  res.w = static_cast<float>(color.a) / 255.0f;
  return res;
}

vec4 premultiply_alpha(const vec4& color) {
  return vec4{ color.x * color.w, color.y * color.w, color.z * color.w, color.w };
}

vec4 to_srgb(const vec4& color) {
  auto res = vec4{ };
  res.x = ::to_srgb(color.x);
  res.y = ::to_srgb(color.y);
  res.z = ::to_srgb(color.z);
  res.w = ::to_srgb(color.w);
  return res;
}

vec4 to_linear(const vec4& color) {
  auto res = vec4{ };
  res.x = ::to_linear(color.x);
  res.y = ::to_linear(color.y);
  res.z = ::to_linear(color.z);
  res.w = ::to_linear(color.w);
  return res;
}

vec4 blend(vec4 a, vec4 b, const bool premultiplied) {
  if (!premultiplied)
  {
    a = premultiply_alpha(a);
    b = premultiply_alpha(b);
  }
  a.x = a.x + b.x * (1.0f - a.w);
  a.y = a.y + b.y * (1.0f - a.w);
  a.z = a.z + b.z * (1.0f - a.w);
  a.w = 1.0f;
  return a;
}

}
