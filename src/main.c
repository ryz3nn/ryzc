#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define RL_GUI_IMPL
#define RL_GUI_PLATFORM_SDL3
#define RL_GUI_BACKEND_GLES2
#include "rl/rl_gui.h"

#define TARGET_FPS 30
const uint64_t time_per_frame = 1000000000ULL / TARGET_FPS;

int main(void) {
  rlgui_DeviceData dd;
  static rlgui_Context ui = {0};

  if (rlgui_init(&dd, "ryzc", 800, 600) != 0) {
    fprintf(stderr, "Init rlgui failed\n");
    return 1;
  }

  bool running = true;
  SDL_Event event;

  while (running) {
    uint64_t time_start_frame = SDL_GetTicksNS();
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    int width, height;
    SDL_GetWindowSizeInPixels(dd.window, &width, &height);

    rlgui_begin(&ui, width, height);
    // Background card
    rlgui_draw_rect(&ui, rlgui_create_rect(0, 0, width, height),
                    rlgui_create_color(30, 34, 42, 255));

    // Title bar
    rlgui_draw_rect(&ui, rlgui_create_rect(0, 0, width, 36), rlgui_create_color(45, 52, 64, 255));

    // Simulated button quads
    rlgui_draw_rect(&ui, rlgui_create_rect(60, 95, 120, 36),
                    rlgui_create_color(66, 133, 244, 255)); // Blue button
    rlgui_draw_rect(&ui, rlgui_create_rect(195, 95, 120, 36),
                    rlgui_create_color(52, 168, 83, 255)); // Green button
    rlgui_draw_rect(&ui, rlgui_create_rect(330, 95, 120, 36),
                    rlgui_create_color(234, 67, 53, 255)); // Red button

    // Divider line
    rlgui_draw_rect(&ui, rlgui_create_rect(60, 155, 460, 2), rlgui_create_color(60, 68, 82, 255));

    // Simulated progress bar (track + fill)
    rlgui_draw_rect(&ui, rlgui_create_rect(60, 180, 460, 18),
                    rlgui_create_color(20, 22, 28, 255)); // Track
    rlgui_draw_rect(&ui, rlgui_create_rect(60, 180, 280, 18),
                    rlgui_create_color(66, 133, 244, 200)); // Fill with alpha

    // Translucent overlay badge
    rlgui_draw_rect(&ui, rlgui_create_rect(60, 220, 180, 50), rlgui_create_color(255, 193, 7, 180));

    // 2. Render

    rlgui_render(&dd, &ui);
    SDL_GL_SwapWindow(dd.window);

    uint64_t elapsed = SDL_GetTicksNS() - time_start_frame;
    if (elapsed < time_per_frame) {
      SDL_DelayNS(time_per_frame - elapsed);
    }
  }

  rlgui_destroy(&dd);
  return 0;
}
