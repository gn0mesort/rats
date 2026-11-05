#ifndef RATS_MATH_HPP
#define RATS_MATH_HPP

#include <cmath>

#include <concepts>
#include <numbers>

namespace rats {

constexpr const float EPSILON{ 1e-6f };

struct vec2 final {
  float x;
  float y;
};

struct vec3 final {
  float x;
  float y;
  float z;
};

struct vec4 final {
  float x;
  float y;
  float z;
  float w;
};

template <std::floating_point Type>
struct tolerances final {
  static constexpr Type absolute_tolerance{ 0 };
  static constexpr Type relative_tolerance{ 1e-9 };
};

template <std::floating_point Type>
constexpr bool is_close(const Type a, const Type b, const Type absolute_tolerance, const Type relative_tolerance) {
  if (a == b)
  {
    return true;
  }
  if (std::isinf(a) || std::isinf(b))
  {
    return false;
  }
  const auto difference = std::fabs(b - a);
  return difference <= std::fabs(relative_tolerance * b) ||
         difference <= std::fabs(relative_tolerance * a) ||
         difference <= absolute_tolerance;
}

template <std::floating_point Type>
constexpr bool is_close(const Type a, const Type b, const Type absolute_tolerance) {
  return is_close(a, b, absolute_tolerance, tolerances<Type>::relative_tolerance);
}

template <std::floating_point Type>
constexpr bool is_close(const Type a, const Type b) {
  return is_close(a, b, tolerances<Type>::absolute_tolerance, tolerances<Type>::relative_tolerance);
}

template <std::floating_point Type>
constexpr bool is_almost_equal(const Type a, const Type b, const Type absolute_tolerance) {
  if (a == b)
  {
    return true;
  }
  if (std::isinf(a) || std::isinf(b))
  {
    return false;
  }
  return std::fabs(b - a) <= absolute_tolerance;
}

template <std::floating_point Type>
constexpr bool is_almost_zero(const Type a, const Type absolute_tolerance) {
  return is_almost_equal(a, Type{ 0 }, absolute_tolerance);
}

template <std::floating_point Type>
constexpr Type radians(const Type degrees) {
  return degrees * std::numbers::pi_v<Type> / Type{ 180.0 };
}

template <std::floating_point Type>
constexpr Type degrees(const Type radians) {
  return radians * Type{ 180.0 } / std::numbers::pi_v<Type>;
}

template <std::floating_point Type>
constexpr Type tau_v{ Type{ 2.0 } * std::numbers::pi_v<Type> };

vec2 operator-(const vec2& a);
vec3 operator-(const vec3& a);
vec4 operator-(const vec4& a);
vec2 operator+(const vec2& a, const vec2& b);
vec3 operator+(const vec3& a, const vec3& b);
vec4 operator+(const vec4& a, const vec4& b);
vec2 operator-(const vec2& a, const vec2& b);
vec3 operator-(const vec3& a, const vec3& b);
vec4 operator-(const vec4& a, const vec4& b);
vec2 operator*(const vec2& a, const vec2& b);
vec3 operator*(const vec3& a, const vec3& b);
vec4 operator*(const vec4& a, const vec4& b);
vec2 operator/(const vec2& a, const vec2& b);
vec3 operator/(const vec3& a, const vec3& b);
vec4 operator/(const vec4& a, const vec4& b);
vec2 operator*(const vec2& a, const float b);
vec3 operator*(const vec3& a, const float b);
vec4 operator*(const vec4& a, const float b);
vec2 operator/(const vec2& a, const float b);
vec3 operator/(const vec3& a, const float b);
vec4 operator/(const vec4& a, const float b);
float dot(const vec2& a, const vec2& b);
float dot(const vec3& a, const vec3& b);
float dot(const vec4& a, const vec4& b);
float cross(const vec2& a, const vec2& b);
vec3 cross(const vec3& a, const vec3& b);
vec2 lerp(const vec2& a, const vec2& b, const float t);
vec3 lerp(const vec3& a, const vec3& b, const float t);
vec4 lerp(const vec4& a, const vec4& b, const float t);
float length(const vec2& a);
float length(const vec3& a);
float length(const vec4& a);
vec2 normalize(const vec2& a);
vec3 normalize(const vec3& a);
vec4 normalize(const vec4& a);

}

#endif
