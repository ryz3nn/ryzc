#pragma once

#include <functional>
#include <string_view>

struct SDL_Window;
struct SDL_GLContextState;

struct App {
  explicit App(std::string_view title = "ryzc", int width = 1280,
               int height = 800);
  ~App();

  // Not allowed copy
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // Allowed moving
  App(App&& other) noexcept;            // Constructor by create new obejct
  App& operator=(App&& other) noexcept; // Create by assign to exist object

  // Check initialization succeeded or not
  [[nodiscard]] bool is_valid() const noexcept { return is_initialized_; }

  int run(const std::function<void()>& ui_callback);

  void cleanup() noexcept;

private:
  SDL_Window* window_ = nullptr;
  SDL_GLContextState* gl_context_ = nullptr;
  bool is_initialized_ = false;
};
