#include "rats/assets.hpp"

#include "config.hpp"

#include <array>
#include <string>
#include <fstream>
#include <utility>

#ifdef CONFIG_MESON_SYSTEM_LINUX
#  include <unistd.h>
#  include <sys/types.h>
#  include <pwd.h>
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
#  include <Windows.h>
#  include <Objbase.h>
#  include <Shlobj.h>
#endif

#include <webp/decode.h>

#define MA_NO_DEVICE_IO
#include <miniaudio.h>

#include "rats/error.hpp"
#include "rats/graphics.hpp"

namespace {

#define CONFIG_MESON_SEARCH_PATH(x) x,

const std::array<std::filesystem::path, CONFIG_MESON_SEARCH_PATH_COUNT> search_paths{ CONFIG_MESON_SEARCH_PATHS };

#undef CONFIG_MESON_SEARCH_PATH

std::filesystem::path expand_path(const std::filesystem::path& path) {
  auto res = std::filesystem::path{ };
  for (const auto& part : path)
  {
    if (part == "~")
    {
      res /= rats::assets::home_directory();
    }
    else if (part == "$ORIGIN")
    {
      res /= rats::assets::executable_directory();
    }
    else
    {
      res /= part;
    }
  }
  return std::filesystem::absolute(res);
}

}

namespace rats::assets {

std::filesystem::path executable_directory() {
#ifdef CONFIG_MESON_SYSTEM_LINUX
  return std::filesystem::absolute(std::filesystem::read_symlink("/proc/self/exe").parent_path());
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
  auto module = GetModuleHandle(nullptr);
  auto rawpath = std::wstring(MAX_PATH, '\0');
  const auto actual_size = GetModuleFileName(module, rawpath.data(), rawpath.size());
  rawpath.resize(actual_size);
  return std::filesystem::absolute(std::filesystem::path{ rawpath }.parent_path());
#else
  throw error{ error::origin::application, "The application couldn't find the program's location." };
#endif
}

std::filesystem::path home_directory() {
#ifdef CONFIG_MESON_SYSTEM_LINUX
  if (const auto *const envp = std::getenv("HOME"))
  {
    return std::filesystem::absolute(envp);
  }
  const auto uid = geteuid();
  auto pwd = passwd{ };
  auto bufsz = std::size_t{ };
  if (const auto res = sysconf(_SC_GETPW_R_SIZE_MAX); res >= 0)
  {
    bufsz = res;
  }
  else
  {
    throw error{ error::origin::application, "The application failed to retrieve the user's home directory." };
  }
  auto buf = std::string(bufsz, '\0');
  {
    auto* res = static_cast<passwd*>(nullptr);
    if (getpwuid_r(uid, &pwd, buf.data(), bufsz, &res) < 0)
    {
      throw error{ error::origin::application, "The application failed to retrieve the user's home directory." };
    }
    if (res)
    {
      return std::filesystem::absolute(res->pw_dir);
    }
  }
  throw error{ error::origin::application, "The application couldn't find the user's home directory." };
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
  auto* rawpath = static_cast<PWSTR>(nullptr);
  if (SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &rawpath) != S_OK)
  {
    CoTaskMemFree(rawpath);
    throw error{ error::origin::application, "The application failed to retrieve the user's home directory." };
  }
  const auto res = std::filesystem::absolute(std::filesystem::path{ rawpath });
  CoTaskMemFree(rawpath);
  return res;
#else
  throw error{ error::origin::application, "The application couldn't find the user's home directory." };
#endif
}

std::optional<std::filesystem::path> find_asset(const std::filesystem::path& path) {
  auto stats = std::filesystem::file_status{ };
  if (path.is_absolute())
  {
    stats = std::filesystem::status(path);
    if (std::filesystem::exists(stats) && !std::filesystem::is_directory(stats))
    {
      return path;
    }
    return std::nullopt;
  }
  auto attempt = std::filesystem::path{ };
  for (const auto& search_path : search_paths)
  {
    attempt = std::filesystem::absolute(expand_path(search_path / path));
    stats = std::filesystem::status(attempt);
    if (std::filesystem::exists(stats) && !std::filesystem::is_directory(stats))
    {
      return attempt;
    }
  }
  return std::nullopt;
}

texture::texture(const extent_2d& extent) :
m_texels(extent.width * extent.height),
m_extent{ extent },
m_premultiplied{ false } { }



texture::texture(const std::filesystem::path& path, const bool linear) :
m_premultiplied{ false } {
  {
    const auto stats = std::filesystem::status(path);
    if (!std::filesystem::exists(stats) || std::filesystem::is_directory(stats))
    {
      throw error{ error::origin::application, "The application failed to load a texture." };
    }
  }
  auto binary = std::vector<unsigned char>{ };
  {
    auto f_in = std::ifstream{ path, std::ios::binary };
    const auto sz = std::filesystem::file_size(path);
    binary.resize(sz);
    f_in.read(reinterpret_cast<char*>(binary.data()), sz);
  }
  auto width = int{ };
  auto height = int{ };
  if (!WebPGetInfo(binary.data(), binary.size(), &width, &height))
  {
    throw error{ error::origin::application, "The application detected an invalid texture format." };
  }
  m_texels.resize(width * height);
  if (!WebPDecodeRGBAInto(binary.data(), binary.size(), reinterpret_cast<unsigned char*>(m_texels.data()),
                          m_texels.size() * sizeof(rgba), width * sizeof(rgba)))
  {
    throw error{ error::origin::application, "The application failed to decode texture data." };
  }
  if (!linear)
  {
    for (auto& texel : m_texels)
    {
      texel = denormalized(to_linear(normalized(texel)));
    }
  }
  m_extent.width = width;
  m_extent.height = height;
}

texture::texture(const std::filesystem::path& path) : texture{ path, false } { }

const rgba& texture::operator[](const offset_2d& coordinates) const {
  return m_texels[coordinates.y * m_extent.width + coordinates.x];
}

rgba& texture::operator[](const offset_2d& coordinates) {
  return m_texels[coordinates.y * m_extent.width + coordinates.x];
}

vec4 texture::sample(const vec2& coordinates) const {
  const auto u = static_cast<std::int64_t>(std::clamp(coordinates.x, 0.0f, 1.0f) * (m_extent.width - 1));
  const auto v = static_cast<std::int64_t>(std::clamp(coordinates.y, 0.0f, 1.0f) * (m_extent.height - 1));
  return normalized(m_texels[v * m_extent.width + u]);
}

const rgba& texture::at(const offset_2d& coordinates) const {
  if (coordinates.x < 0 || coordinates.x >= m_extent.width || coordinates.y < 0 || coordinates.y >= m_extent.height)
  {
    throw error{ error::origin::application, "The input texture coordinates are out of bounds." };
  }
  return (*this)[coordinates];
}

rgba& texture::at(const offset_2d& coordinates) {
  if (coordinates.x < 0 || coordinates.x >= m_extent.width || coordinates.y < 0 || coordinates.y >= m_extent.height)
  {
    throw error{ error::origin::application, "The input texture coordinates are out of bounds." };
  }
  return (*this)[coordinates];
}

bool texture::has_premultiplied_alpha() const {
  return m_premultiplied;
}

void texture::premultiply_alpha() {
  for (auto& texel : m_texels)
  {
    texel = denormalized(rats::premultiply_alpha(normalized(texel)));
  }
  m_premultiplied = true;
}

const extent_2d& texture::extent() const {
  return m_extent;
}

const rgba& palette_2bit::color(const std::size_t index) const {
  return m_colors.at(index);
}

texture palette_2bit::apply_to(const texture& base) const {
  auto res = texture{ base.extent() };
  const auto* color = static_cast<const rgba*>(nullptr);
  auto coords = offset_2d{ };
  for (auto i = std::uint16_t{ 0 }; i < res.extent().height; ++i)
  {
    for (auto j = std::uint16_t{ 0 }; j < res.extent().width; ++j)
    {
      coords.x = static_cast<std::int16_t>(j);
      coords.y = static_cast<std::int16_t>(i);
      color = &base.at(coords);
      if (color->r == 0 && color->g == 0 && color->b == 0)
      {
        res.at(coords) = rgba{ m_colors[0].r, m_colors[0].g, m_colors[0].b, color->a };
      }
      else if (color->r == 0x3f && color->g == 0x3f && color->b == 0x3f)
      {
        res.at(coords) = rgba{ m_colors[1].r, m_colors[1].g, m_colors[1].b, color->a };
      }
      else if (color->r == 0x7f && color->g == 0x7f && color->b == 0x7f)
      {
        res.at(coords) = rgba{ m_colors[2].r, m_colors[2].g, m_colors[2].b, color->a };
      }
      else if (color->r == 0xff && color->g == 0xff && color->b == 0xff)
      {
        res.at(coords) = rgba{ m_colors[3].r, m_colors[3].g, m_colors[3].b, color->a };
      }
      else
      {
        throw error{ error::origin::application, "The input texture contains non-palette compatible colors." };
      }
    }
  }
  return res;
}

  sound::sound(const std::filesystem::path& path) {
    {
      const auto stats = std::filesystem::status(path);
      if (!std::filesystem::exists(stats) || std::filesystem::is_directory(stats))
      {
        throw error{ error::origin::application, "The application failed to load an audio file." };
      }
    }
    const auto config = ma_decoder_config_init(ma_format_f32, 2, 48000);
    auto decoder = ma_decoder{ };
#ifdef CONFIG_MESON_SYSTEM_LINUX
    if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS)
#elif CONFIG_MESON_SYSTEM_WINDOWS
    if (ma_decoder_init_file_w(path.c_str(), &config, &decoder) != MA_SUCCESS)
#endif
    {
      throw error{ error::origin::application, "The application failed to create an audio decoder." };
    }
    auto frames = ma_uint64{ };
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frames) != MA_SUCCESS ||
        frames > (m_samples.max_size() >> 1ULL))
    {
      ma_decoder_uninit(&decoder);
      throw error{ error::origin::application, "The application failed to determine the length of an audio file." };
    }
    m_frame_count = frames;
    m_samples.resize(m_frame_count << 1ULL);
    if (ma_decoder_read_pcm_frames(&decoder, m_samples.data(), m_frame_count, &frames) != MA_SUCCESS ||
        frames < m_frame_count)
    {
      ma_decoder_uninit(&decoder);
      throw error{ error::origin::application, "The application failed to decode an audio file." };
    }
    ma_decoder_uninit(&decoder);
  }

  sound::sound(sound&& other) :
  m_samples{ std::move(other.m_samples) },
  m_frame_count{ std::exchange(other.m_frame_count, 0) } { }

  sound& sound::operator=(sound&& rhs) {
    if (this != &rhs)
    {
      m_samples = std::move(rhs.m_samples);
      m_frame_count = std::exchange(rhs.m_frame_count, 0);
    }
    return *this;
  }

  std::size_t sound::frame_count() const {
    return m_frame_count;
  }

  const std::vector<float>& sound::samples() const {
    return m_samples;
  }

}
