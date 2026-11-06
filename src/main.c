#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

#include <stdbool.h>
#include <stdio.h>

#include "gui/renderer.h"
#include "gui/rui.h"

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_Window *window =
      SDL_CreateWindow("ryzc UI - Rectangles Only", 800, 600,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!window) {
    fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) {
    fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_SetSwapInterval(1);

  rui_Context ui;
  rui_init(&ui);

  if (!rui_renderer_init()) {
    fprintf(stderr, "Failed to initialize rui renderer\n");
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
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
    SDL_GetWindowSizeInPixels(window, &width, &height);

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
    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    rui_renderer_render(&ui);

    SDL_GL_SwapWindow(window);
  }

  rui_renderer_destroy();
  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
