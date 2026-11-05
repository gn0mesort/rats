#ifndef RATS_SDL3_MIXER_HPP
#define RATS_SDL3_MIXER_HPP

#include <cstddef>

#include <memory>

namespace rats::assets {

  class sound;

}

namespace rats::sdl3 {

  class library;

  class mixer final {
  private:
    struct info;

    std::shared_ptr<library> m_parent;
    std::unique_ptr<info> m_info;
  public:
    mixer(const std::shared_ptr<library>& parent, const std::size_t voices, const std::size_t tracks);
    explicit mixer(const std::shared_ptr<library>& parent);
    mixer(const mixer& other) = delete;
    mixer(mixer&& other) = delete;

    ~mixer() noexcept;

    mixer& operator=(const mixer& rhs) = delete;
    mixer& operator=(mixer&& rhs) = delete;
    std::size_t play(const assets::sound& snd);
    void play_on_voice(const std::size_t index, const assets::sound& snd, const bool force = false);
    void play_on_track(const std::size_t index, const assets::sound& snd, const bool force = false);
    void loop_on_track(const std::size_t index, const assets::sound& snd, const bool force = false);
    void stop(const std::size_t index);
    void stop_track(const std::size_t index);
    void process();
  };

}

#endif
