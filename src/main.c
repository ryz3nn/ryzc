#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Không thể khởi tạo SDL: %s", SDL_GetError());
    return -1;
  }

  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

  if (!SDL_CreateWindowAndRenderer("Hello SDL3", 640, 480, 0, &window,
                                   &renderer)) {
    SDL_Log("Không thể tạo cửa sổ và renderer: %s", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  bool running = true;
  SDL_Event event;
  Uint64 start_time = SDL_GetTicks();

  // Run event loop for 3 seconds or until closed
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    // Clear and present a frame so the compositor maps the window
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Delay(16); // Cap polling rate (~60 FPS)
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
