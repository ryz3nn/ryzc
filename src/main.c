#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

#include <stdbool.h>
#include <stdio.h>

#include "gui/renderer.h"

int main(void) {
  rui_Context ui;
  rui_init(&ui);

  rui_renderer_BackendData bd;
  bd.backend_type = RUI_RENDERER_GLES2;
  bd.window_properties = (rui_renderer_WindowProperties){.name = "ryzc", .width = 800, .height = 600};

  if (!rui_renderer_init(&bd)) {
    fprintf(stderr, "Failed to initialize rui renderer\n");
    return 1;
  }

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    int width, height;
    SDL_GetWindowSizeInPixels(bd.window, &width, &height);

    // 1. Build UI frame with rects
    rui_begin(&ui, width, height);

    // Background card
    rui_draw_rect(&ui, rui_create_rect(0, 0, width, height),
                  rui_create_color(30, 34, 42, 255));

    // Title bar
    rui_draw_rect(&ui, rui_create_rect(0, 0, width, 36),
                  rui_create_color(45, 52, 64, 255));

    // Simulated button quads
    rui_draw_rect(&ui, rui_create_rect(60, 95, 120, 36),
                  rui_create_color(66, 133, 244, 255)); // Blue button
    rui_draw_rect(&ui, rui_create_rect(195, 95, 120, 36),
                  rui_create_color(52, 168, 83, 255)); // Green button
    rui_draw_rect(&ui, rui_create_rect(330, 95, 120, 36),
                  rui_create_color(234, 67, 53, 255)); // Red button

    // Divider line
    rui_draw_rect(&ui, rui_create_rect(60, 155, 460, 2),
                  rui_create_color(60, 68, 82, 255));

    // Simulated progress bar (track + fill)
    rui_draw_rect(&ui, rui_create_rect(60, 180, 460, 18),
                  rui_create_color(20, 22, 28, 255)); // Track
    rui_draw_rect(&ui, rui_create_rect(60, 180, 280, 18),
                  rui_create_color(66, 133, 244, 200)); // Fill with alpha

    // Translucent overlay badge
    rui_draw_rect(&ui, rui_create_rect(60, 220, 180, 50),
                  rui_create_color(255, 193, 7, 180));

    rui_end(&ui);

    // 2. Render
    
    rui_renderer_draw(&ui, &bd);

    SDL_GL_SwapWindow(bd.window);
  }

  rui_renderer_destroy(&bd);
  return 0;
}
