#include "app.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>

bool App::init(const char* title, int32_t width, int32_t height) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  const SDL_WindowFlags window_flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

  window = SDL_CreateWindow(
      title,
      static_cast<int>(static_cast<float>(width) * main_scale),
      static_cast<int>(static_cast<float>(height) * main_scale),
      window_flags);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return false;
  }

  gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsLight();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 130");

  is_running = true;
  return true;
}

bool App::begin_frame() {
  if (!is_running) {
    return false;
  }

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_QUIT) {
      is_running = false;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window)) {
      is_running = false;
    }
  }

  if (!is_running) {
    return false;
  }

  if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return false;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  return true;
}

void App::end_frame() {
  ImGui::Render();
  const ImGuiIO& imgui_io = ImGui::GetIO();
  glViewport(0, 0, static_cast<int>(imgui_io.DisplaySize.x),
             static_cast<int>(imgui_io.DisplaySize.y));
  glClearColor(0.85f, 0.85f, 0.85f, 0.85f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(window);
}

void App::shutdown() {
  if (window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (gl_context) {
      SDL_GL_DestroyContext(gl_context);
      gl_context = nullptr;
    }

    SDL_DestroyWindow(window);
    window = nullptr;
  }

  SDL_Quit();
  is_running = false;
}
