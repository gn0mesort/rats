#include "rats/sdl3/mixer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#define MA_NO_DEVICE_IO
#include <miniaudio.h>

#include "rats/error.hpp"
#include "rats/logger.hpp"
#include "rats/assets.hpp"

#include "rats/sdl3/library.hpp"

#define SDL3_CHECK(exp, ...) \
  do \
  { \
    if (!(exp)) \
    { \
      __VA_ARGS__ \
      throw rats::error{ rats::error::origin::sdl3, SDL_GetError() }; \
    } \
  } \
  while (0)

namespace {

  struct voice final {
    ma_audio_buffer_config config;
    ma_audio_buffer buffer;
    ma_sound sound;
    bool busy;
  };

  std::string audio_format_to_string(const SDL_AudioFormat format) {
    switch (format)
    {
    case SDL_AUDIO_U8:
      return "Unsigned 8-bit PCM";
    case SDL_AUDIO_S8:
      return "Signed 8-bit PCM";
    case SDL_AUDIO_S16LE:
      return "Signed 16-bit PCM LE";
    case SDL_AUDIO_S16BE:
      return "Signed 16-bit PCM BE";
    case SDL_AUDIO_S32LE:
      return "Signed 32-bit PCM LE";
    case SDL_AUDIO_S32BE:
      return "Signed 32-bit PCM BE";
    case SDL_AUDIO_F32LE:
      return "Float 32-bit PCM LE";
    case SDL_AUDIO_F32BE:
      return "Float 32-bit PCM BE";
    default:
      return "Unknown";
    }
  }

}

namespace rats::sdl3 {

  struct mixer::info final {
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_specification;
    SDL_AudioStream* audio_stream;
    ma_engine audio_engine;
    std::vector<voice> audio_voices;
    std::vector<voice> audio_tracks;
  };

  mixer::mixer(const std::shared_ptr<library>& parent, const std::size_t voices, const std::size_t tracks) :
  m_parent{ parent },
  m_info{ new info{ } } {
    if (!parent)
    {
      throw error{ error::origin::sdl3, "The application can only construct a mixer from a parent library." };
    }
    if (!voices || !tracks)
    {
      throw error{ error::origin::sdl3, "A mixer must have at least 1 voice slot and at least 1 track slot." };
    }
    constexpr const auto CHANNELS{ 2 };
    constexpr const auto FREQUENCY{ 48000 };
    m_info->audio_specification = SDL_AudioSpec{ SDL_AUDIO_F32, CHANNELS, FREQUENCY };
     m_info->audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_info->audio_specification);
    SDL3_CHECK(m_info->audio_device);
    SDL3_CHECK(SDL_GetAudioDeviceFormat(m_info->audio_device, &m_info->audio_specification, nullptr),
               { SDL_CloseAudioDevice(m_info->audio_device); });
    RATS_INFO(to_string(error::origin::sdl3), "Audio Specification: {}, {} channels @ {} Hz",
              audio_format_to_string(m_info->audio_specification.format), m_info->audio_specification.channels,
              m_info->audio_specification.freq);
    auto config = ma_engine_config_init();
    config.noDevice = true;
    config.channels = CHANNELS;
    config.sampleRate = FREQUENCY;
    if (ma_engine_init(&config, &m_info->audio_engine) != MA_SUCCESS)
    {
      SDL_CloseAudioDevice(m_info->audio_device);
      throw error{ error::origin::sdl3, "The application failed to initialize Miniaudio alongside SDL3." };
    }
    const auto from = SDL_AudioSpec{ SDL_AUDIO_F32, CHANNELS, FREQUENCY };
    m_info->audio_stream = SDL_CreateAudioStream(&from, &m_info->audio_specification);
    SDL3_CHECK(m_info->audio_stream,
               { ma_engine_uninit(&m_info->audio_engine); SDL_CloseAudioDevice(m_info->audio_device); });
    SDL3_CHECK(SDL_BindAudioStream(m_info->audio_device, m_info->audio_stream),
               { SDL_DestroyAudioStream(m_info->audio_stream); ma_engine_uninit(&m_info->audio_engine);
                 SDL_CloseAudioDevice(m_info->audio_device); });\
    m_info->audio_voices.resize(voices);
    m_info->audio_tracks.resize(tracks);
    for (auto& voice : m_info->audio_voices)
    {
      voice.busy = false;
    }
    for (auto& voice : m_info->audio_tracks)
    {
      voice.busy = false;
    }
  }

  mixer::mixer(const std::shared_ptr<library>& parent) :
  mixer{ parent, 32, 2 } { }

  mixer::~mixer() noexcept {
    for (auto& voice : m_info->audio_voices)
    {
      if (voice.busy)
      {
        ma_sound_stop(&voice.sound);
        ma_sound_uninit(&voice.sound);
        ma_audio_buffer_uninit(&voice.buffer);
      }
    }
    for (auto& voice : m_info->audio_tracks)
    {
      if (voice.busy)
      {
        ma_sound_stop(&voice.sound);
        ma_sound_uninit(&voice.sound);
        ma_audio_buffer_uninit(&voice.buffer);
      }
    }
    // Do this first to block until audio finishes.
    SDL_CloseAudioDevice(m_info->audio_device);
    SDL_DestroyAudioStream(m_info->audio_stream);
    ma_engine_uninit(&m_info->audio_engine);
  }

  std::size_t mixer::play(const assets::sound& snd) {
    auto res = std::size_t{ 0 };
    for (auto& voice : m_info->audio_voices)
    {
      if (!voice.busy)
      {
        voice.config = ma_audio_buffer_config_init(ma_format_f32, 2, snd.frame_count(), snd.samples().data(), nullptr);
        voice.config.sampleRate = 48000;
        if (ma_audio_buffer_init(&voice.config, &voice.buffer) != MA_SUCCESS)
        {
          throw error{ error::origin::sdl3, "The application failed to play a sound." };
        }
        if (ma_sound_init_from_data_source(&m_info->audio_engine, reinterpret_cast<ma_data_source*>(&voice.buffer),
                                           0, nullptr, &voice.sound) != MA_SUCCESS)
        {
          ma_audio_buffer_uninit(&voice.buffer);
          throw error{ error::origin::sdl3, "The application failed to play a sound." };
        }
        if (ma_sound_start(&voice.sound) != MA_SUCCESS)
        {
          ma_audio_buffer_uninit(&voice.buffer);
          ma_sound_uninit(&voice.sound);
          throw error{ error::origin::sdl3, "The application failed to play a sound." };
        }
        voice.busy = true;
        return res;
      }
      ++res;
    }
    return m_info->audio_voices.size();
  }

  void mixer::play_on_voice(const std::size_t index, const assets::sound& snd, const bool force) {
    auto& voice = m_info->audio_voices.at(index);
    if (voice.busy)
    {
      if (!force)
      {
        return;
      }
      stop(index);
    }
    voice.config = ma_audio_buffer_config_init(ma_format_f32, 2, snd.frame_count(), snd.samples().data(), nullptr);
    voice.config.sampleRate = 48000;
    if (ma_audio_buffer_init(&voice.config, &voice.buffer) != MA_SUCCESS)
    {
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    if (ma_sound_init_from_data_source(&m_info->audio_engine, reinterpret_cast<ma_data_source*>(&voice.buffer), 0,
                                       nullptr, &voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    if (ma_sound_start(&voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      ma_sound_uninit(&voice.sound);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    voice.busy = true;
  }

  void mixer::play_on_track(const std::size_t index, const assets::sound& snd, const bool force) {
    auto& voice = m_info->audio_tracks.at(index);
    if (voice.busy)
    {
      if (!force)
      {
        return;
      }
      stop_track(index);
    }
    voice.config = ma_audio_buffer_config_init(ma_format_f32, 2, snd.frame_count(), snd.samples().data(), nullptr);
    voice.config.sampleRate = 48000;
    if (ma_audio_buffer_init(&voice.config, &voice.buffer) != MA_SUCCESS)
    {
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    if (ma_sound_init_from_data_source(&m_info->audio_engine, reinterpret_cast<ma_data_source*>(&voice.buffer), 0,
                                       nullptr, &voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    if (ma_sound_start(&voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      ma_sound_uninit(&voice.sound);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    voice.busy = true;
  }

  void mixer::loop_on_track(const std::size_t index, const assets::sound& snd, const bool force) {
    auto& voice = m_info->audio_tracks.at(index);
    if (voice.busy)
    {
      if (!force)
      {
        return;
      }
      stop_track(index);
    }
    voice.config = ma_audio_buffer_config_init(ma_format_f32, 2, snd.frame_count(), snd.samples().data(), nullptr);
    voice.config.sampleRate = 48000;
    if (ma_audio_buffer_init(&voice.config, &voice.buffer) != MA_SUCCESS)
    {
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    if (ma_sound_init_from_data_source(&m_info->audio_engine, reinterpret_cast<ma_data_source*>(&voice.buffer), 0,
                                       nullptr, &voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    ma_sound_set_looping(&voice.sound, true);
    if (ma_sound_start(&voice.sound) != MA_SUCCESS)
    {
      ma_audio_buffer_uninit(&voice.buffer);
      ma_sound_uninit(&voice.sound);
      throw error{ error::origin::sdl3, "The application failed to play a sound." };
    }
    voice.busy = true;
  }

  void mixer::stop(const std::size_t index) {
    auto& voice = m_info->audio_voices.at(index);
    if (voice.busy)
    {
      ma_sound_stop(&voice.sound);
      ma_sound_uninit(&voice.sound);
      ma_audio_buffer_uninit(&voice.buffer);
      voice.busy = false;
    }
  }

  void mixer::stop_track(const std::size_t index) {
    auto& voice = m_info->audio_tracks.at(index);
    if (voice.busy)
    {
      ma_sound_stop(&voice.sound);
      ma_sound_uninit(&voice.sound);
      ma_audio_buffer_uninit(&voice.buffer);
      voice.busy = false;
    }
  }

  void mixer::process() {
    for (auto& voice : m_info->audio_voices)
    {
      if (!voice.busy)
      {
        continue;
      }
      if (!ma_sound_is_playing(&voice.sound))
      {
        ma_sound_uninit(&voice.sound);
        ma_audio_buffer_uninit(&voice.buffer);
        voice.busy = false;
      }
    }
    for (auto& voice : m_info->audio_tracks)
    {
      if (!voice.busy)
      {
        continue;
      }
      if (!ma_sound_is_playing(&voice.sound))
      {
        ma_sound_uninit(&voice.sound);
        ma_audio_buffer_uninit(&voice.buffer);
        voice.busy = false;
      }
    }
    const auto queued = SDL_GetAudioStreamQueued(m_info->audio_stream);
    if (queued < 0)
    {
      throw error{ error::origin::sdl3, SDL_GetError() };
    }
    auto queued_frames = static_cast<std::size_t>(queued) / (sizeof(float) << 1ULL);
    auto output = std::array<float, 2048>{ };
    while (queued_frames < 4096)
    {
      if (ma_engine_read_pcm_frames(&m_info->audio_engine, output.data(), output.size() >> 1ULL,
                                    nullptr) != MA_SUCCESS)
      {
        throw error{ error::origin::sdl3, "The application failed to render audio." };
      }
      SDL3_CHECK(SDL_PutAudioStreamData(m_info->audio_stream, output.data(), output.size() * sizeof(float)));
      queued_frames += output.size() >> 1ULL;
    }
  }

}
