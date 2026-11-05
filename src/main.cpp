#include <cstring>
#include <cmath>

#include <iostream>
#include <numbers>
#include <chrono>
#include <algorithm>
#include <array>
#include <numeric>

#include "rats/error.hpp"
#include "rats/sdl3.hpp"
#include "rats/renderer.hpp"
#include "rats/logger.hpp"
#include "rats/stopwatch.hpp"
#include "rats/timer.hpp"
#include "rats/world.hpp"
#include "rats/math.hpp"
#include "rats/assets.hpp"
#include "rats/easing.hpp"
#include "rats/sprite.hpp"

#include "config.hpp"

#define TITLE \
  "Rats! - " CONFIG_MESON_PROJECT_VERSION_STRING " - " CONFIG_MESON_CPU_FAMILY_ID " - " CONFIG_MESON_SYSTEM_ID

int main() {
  RATS_INFO(rats::to_string(rats::error::origin::application), TITLE);
  try
  {
    RATS_INFO(rats::to_string(rats::error::origin::application), "ORIGIN: {}", rats::assets::executable_directory().string());
    RATS_INFO(rats::to_string(rats::error::origin::application), "HOME: {}", rats::assets::home_directory().string());
    auto generator = rats::world_generator{ };
    auto world = generator.generate({ 32, 32 });
    auto sdl3 = std::shared_ptr<rats::sdl3::library>{ new rats::sdl3::library{ } };
    auto window = std::unique_ptr<rats::sdl3::window>{ };
    {
      const auto resolution = sdl3->primary_display_resolution();
      auto frame = rats::rect_2d{ };
      if (resolution.width > 1920 && resolution.height > 1080)
      {
        frame.extent.width = 1920;
        frame.extent.height = 1080;
      }
      else if (resolution.width > 1280 && resolution.height > 720)
      {
        frame.extent.width = 1280;
        frame.extent.height = 720;
      }
      else if (resolution.width > 640 && resolution.height > 360)
      {
        frame.extent.width = 640;
        frame.extent.height = 360;
      }
      else
      {
        frame.extent.width = resolution.width - (resolution.width * 0.1f);
        frame.extent.height = resolution.height - (resolution.height * 0.1f);
      }
      frame.offset.x = (resolution.width - frame.extent.width) >> 1;
      frame.offset.y = (resolution.height - frame.extent.height) >> 1;
      window.reset(new rats::sdl3::window{ sdl3, TITLE, frame });
    }
    window->make_cursor_relative();
    auto mixer = std::unique_ptr<rats::sdl3::mixer>{ new rats::sdl3::mixer{ sdl3 } };
    auto event = std::optional<rats::sdl3::event>{ };
    auto quitting = false;
    window->make_current();
    window->change_presentation_mode(rats::sdl3::window::presentation_mode::relaxed);
    window->show();
    auto renderer = rats::renderer{ std::move(window), { 320, 180 } };
    auto hz_60_timer = rats::timer{ 1.0 / 60.0 };
    auto logger_flush_timer = rats::timer{ 1.0 };
    auto dt = rats::stopwatch::duration_type{ };
    auto sw = rats::stopwatch{ };
    world.add_player(rats::entity{ rats::vec2{ world.start().x + 0.5f, world.start().y + 0.5f }, 0.0f, nullptr,
                                   rats::entity_type::player });
    auto turn_l = false;
    auto turn_r = false;
    auto debug_typeface = rats::assets::texture{ *rats::assets::find_asset("assets/img/debug.webp") };
    debug_typeface.premultiply_alpha();
    auto typeface = rats::assets::texture{ *rats::assets::find_asset("assets/img/ascii.webp"), true };
    typeface = rats::world::palette().apply_to(typeface);
    typeface.premultiply_alpha();
    auto reticle = rats::assets::texture{ *rats::assets::find_asset("assets/img/reticle.webp"), true };
    reticle = rats::world::palette().apply_to(reticle);
    reticle.premultiply_alpha();
    auto heart = rats::assets::texture{ *rats::assets::find_asset("assets/img/heart.webp"), true };
    heart = rats::world::palette().apply_to(heart);
    heart.premultiply_alpha();
    auto indicator = rats::sprite{ heart, { 320.0f - 50.5f, 1.5f } };
    indicator.intern_animation("pulse",
                               { rats::stopwatch::duration_type{ 0.5 }, { { 0, 0 }, { 48, 16 } }, { 16, 16 }, 3 });
    indicator.play("pulse", true);
    auto avg_dt = double{ 0.0 };
    auto draw_stats = false;
    auto music = rats::assets::sound{ *rats::assets::find_asset("assets/snd/theme.mp3") };
    mixer->loop_on_track(0, music);
    while (!quitting)
    {
      while ((event = sdl3->poll()))
      {
        switch (event->type)
        {
        case rats::sdl3::event_type::quit:
          quitting = true;
          break;
        case rats::sdl3::event_type::key:
          if (world.player().hp() && event->key.scancode == rats::sdl3::scancode::key_q)
          {
            turn_l = event->key.pressed;
          }
          if (world.player().hp() && event->key.scancode == rats::sdl3::scancode::key_e)
          {
            turn_r = event->key.pressed;
          }
          if (world.player().hp() && event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_w)
          {
            world.walk_player_to(world.player().position() + world.player().forward(), *mixer);
          }
          if (world.player().hp() && event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_s)
          {
            world.walk_player_to(world.player().position() - world.player().forward(), *mixer);
          }
          if (world.player().hp() && event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_a)
          {
            world.walk_player_to(world.player().position() - world.player().right(), *mixer);
          }
          if (world.player().hp() && event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_d)
          {
            world.walk_player_to(world.player().position() + world.player().right(), *mixer);
          }
          if (world.player().hp() && event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_tab)
          {
            world.toggle_automap();
          }
          if (event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_grave)
          {
            draw_stats = !draw_stats;
          }
          if (event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_rctrl)
          {
            renderer.window().make_cursor_absolute();
          }
          if (event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_escape)
          {
            quitting = true;
          }
          if (event->key.pressed && event->key.scancode == rats::sdl3::scancode::key_return &&
              event->key.modifiers & rats::sdl3::modifier::alt)
          {
            renderer.window().toggle_fullscreen();
          }
          break;
        case rats::sdl3::event_type::mouse_motion:
          if (world.player().hp())
          {
            world.rotate_player_by(-event->mouse_motion.relative_x * dt.count());
          }
          break;
        case rats::sdl3::event_type::mouse_button:
          if (world.player().hp() && event->mouse_button.pressed && event->mouse_button.button == rats::sdl3::mouse_button::left)
          {
            world.have_player_attack(*mixer);
          }
          break;
        case rats::sdl3::event_type::window_focus_gained:
          renderer.window().make_cursor_relative();
          break;
        case rats::sdl3::event_type::window_focus_lost:
          renderer.window().make_cursor_absolute();
          break;
        case rats::sdl3::event_type::window_resized:
          renderer.apply_viewport();
          break;
        default:
          break;
        }
      }
      mixer->process();
      while (logger_flush_timer.consume())
      {
        rats::get_thread_logger().flush();
      }
      indicator.update(dt);
      world.update(dt, *mixer);
      if (turn_l)
      {
        world.rotate_player_by(rats::tau_v<float> * dt.count() * 0.5f);
      }
      if (turn_r)
      {
        world.rotate_player_by(-rats::tau_v<float> * dt.count() * 0.5f);
      }
      auto frame = renderer.begin_rendering();
      frame.render(world);
      frame.render(indicator);
      frame.text({ indicator.position().x + 17.0f, 1.5f }, { 8, 16 }, typeface,
                 std::format("{:3}%", world.player().hp()));
      frame.text({ 5.5f, 1.5f }, { 8, 16 }, typeface,
                 std::format("{:09}", world.player().score()));
      if (!world.is_showing_automap())
      {
        if (!world.player().hp())
        {
          frame.text({ 120.5f, 82.5f }, { 8, 16 }, typeface, "Game Over!" );
        }
        else
        {
          frame.blit(reticle, { { 0, 0 }, reticle.extent() },
                     { { static_cast<std::int16_t>((frame.extent().width - reticle.extent().width) >> 1),
                         static_cast<std::int16_t>((frame.extent().height - reticle.extent().height) >> 1) },
                          reticle.extent() });
        }
      }
      if (draw_stats)
      {
        frame.text({ 1.5f, 1.5f, }, { 8, 16 }, debug_typeface, std::format("dt = {:.06f} s", dt.count()));
        frame.text({ 1.5f, 17.5f, }, { 8, 16 }, debug_typeface, std::format("{:.02f} Hz", 1.0 / avg_dt));
        frame.text({ 1.5f, 33.5f, }, { 8, 16 }, debug_typeface, std::format("({:.01f}, {:.01f})",
                                                                            world.player().position().x,
                                                                            world.player().position().y));
        frame.text({ 1.5f, 49.5f, }, { 8, 16 }, debug_typeface, std::format("{:.02f} degrees",
                                                                            rats::degrees(world.player().theta())));
      }
      renderer.end_rendering(std::move(frame));
      renderer.window().present();
      dt = sw.reset();
      logger_flush_timer.update(dt);
      hz_60_timer.update(dt);
      avg_dt = 0.995 * avg_dt + 0.005 * dt.count();
    }
  }
  catch (const rats::error& err)
  {
    RATS_ERROR(rats::to_string(err.origin()), err.message());
    return 1;
  }
  catch (const std::exception& err)
  {
    RATS_ERROR("unknown", err.what());
    return 1;
  }
  catch (...)
  {
    RATS_ERROR("unknown", "The application caught a non-exception type.");
    return 1;
  }
  return 0;
}
