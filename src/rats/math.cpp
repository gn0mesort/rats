#include "rats/math.hpp"

namespace rats {

vec2 operator-(const vec2& a) {
  return { -a.x, -a.y };
}

vec3 operator-(const vec3& a) {
  return { -a.x, -a.y, -a.z };
}

vec4 operator-(const vec4& a) {
  return { -a.x, -a.y, -a.z, -a.w };
}

vec2 operator+(const vec2& a, const vec2& b) {
  return { a.x + b.x, a.y + b.y };
}

vec3 operator+(const vec3& a, const vec3& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z };
}

vec4 operator+(const vec4& a, const vec4& b) {
  return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

vec2 operator-(const vec2& a, const vec2& b) {
  return { a.x - b.x, a.y - b.y };
}

vec3 operator-(const vec3& a, const vec3& b) {
  return { a.x - b.x, a.y - b.y, a.z - b.z };
}

vec4 operator-(const vec4& a, const vec4& b) {
  return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

vec2 operator*(const vec2& a, const vec2& b) {
  return { a.x * b.x, a.y * b.y };
}

vec3 operator*(const vec3& a, const vec3& b) {
  return { a.x * b.x, a.y * b.y, a.z * b.z };
}

vec4 operator*(const vec4& a, const vec4& b) {
  return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

vec2 operator/(const vec2& a, const vec2& b) {
  return { a.x / b.x, a.y / b.y };
}

vec3 operator/(const vec3& a, const vec3& b) {
  return { a.x / b.x, a.y / b.y, a.z / b.z };
}
vec4 operator/(const vec4& a, const vec4& b) {
  return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

vec2 operator*(const vec2& a, const float b) {
  return { a.x * b, a.y * b };
}

vec3 operator*(const vec3& a, const float b) {
  return { a.x * b, a.y * b, a.z * b };
}

vec4 operator*(const vec4& a, const float b) {
  return { a.x * b, a.y * b, a.z * b, a.w * b };
}

vec2 operator/(const vec2& a, const float b) {
  return { a.x / b, a.y / b };
}

vec3 operator/(const vec3& a, const float b) {
  return { a.x / b, a.y / b, a.z / b };
}

vec4 operator/(const vec4& a, const float b) {
  return { a.x / b, a.y / b, a.z / b, a.w / b };
}

float dot(const vec2& a, const vec2& b) {
  return a.x * b.x + a.y * b.y;
}

float dot(const vec3& a, const vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float dot(const vec4& a, const vec4& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float cross(const vec2& a, const vec2& b) {
  return a.x * b.y - a.y * b.x;
}

vec3 cross(const vec3& a, const vec3& b) {
  return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

vec2 lerp(const vec2& a, const vec2& b, const float t) {
  return a + (b - a) * t;
}

vec3 lerp(const vec3& a, const vec3& b, const float t) {
  return a + (b - a) * t;
}

vec4 lerp(const vec4& a, const vec4& b, const float t) {
  return a + (b - a) * t;
}

float length(const vec2& a) {
  return std::sqrt(a.x * a.x + a.y * a.y);
}

float length(const vec3& a) {
  return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

float length(const vec4& a) {
  return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

vec2 normalize(const vec2& a) {
  return a / length(a);
}

vec3 normalize(const vec3& a) {
  return a / length(a);
}

vec4 normalize(const vec4& a) {
  return a / length(a);
}

}
