#pragma once

#include <stdint.h>

struct SDL_Window;
typedef struct SDL_GLContextState* SDL_GLContext;

struct App {
  SDL_Window* window = nullptr;
  SDL_GLContext gl_context = nullptr;
  bool is_running = false;

  bool init(const char* title = "ryzc", int32_t width = 1280,
            int32_t height = 800);
  bool begin_frame();
  void end_frame();
  void shutdown();
};
