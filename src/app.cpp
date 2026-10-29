#include "app.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <SDL3/SDL_video.h>
#include <format>
#include <iostream>
#include <utility>

App::App(std::string_view title, int width, int height) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << std::format("SDL_Init Failed: {}\n", SDL_GetError());
    return;
  }

  // After init success, set some attributes
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  const SDL_WindowFlags window_flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

  window_ = SDL_CreateWindow(
      std::string(title).c_str(),
      static_cast<int>(static_cast<float>(width) * main_scale),
      static_cast<int>(static_cast<float>(height) * main_scale), window_flags);
  if (window_ == nullptr) {
    std::cerr << std::format("SDL_CreateWindow Failed: {}\n", SDL_GetError());
    return;
  }

  gl_context_ = SDL_GL_CreateContext(window_);
  if (gl_context_ == nullptr) {
    std::cerr << std::format("SDL_GL_CreateContext Failed: {}\n",
                             SDL_GetError());
    return;
  }

  // Show window option
  SDL_GL_MakeCurrent(window_, gl_context_);
  SDL_GL_SetSwapInterval(1); // Enable vsync
  SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window_);

  // ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& imgui_io = ImGui::GetIO();
  (void)imgui_io;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // ImGui Theme and Font
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsLight();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale); // Scale UI widget dimensions
  style.FontScaleDpi = main_scale; // Scale default font rendering

  // Init Platform/Render in ImGui
  ImGui_ImplSDL3_InitForOpenGL(window_, gl_context_);
  ImGui_ImplOpenGL3_Init("#version 130");

  is_initialized_ = true;
}

void App::cleanup() noexcept {
  if (is_initialized_) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (gl_context_) {
      SDL_GL_DestroyContext(gl_context_);
    }

    if (window_) {
      SDL_DestroyWindow(window_);
    }
  }

  is_initialized_ = false;
}

App::~App() { cleanup(); }

App::App(App&& other) noexcept
    : window_(std::exchange(other.window_, nullptr)),
      gl_context_(std::exchange(other.gl_context_, nullptr)),
      is_initialized_(std::exchange(other.is_initialized_, false)) {}

App& App::operator=(App&& other) noexcept {
  if (this != &other) {
    // Destroy current data
    cleanup();
    // Steal other data
    window_ = std::exchange(other.window_, nullptr);
    gl_context_ = std::exchange(other.gl_context_, nullptr);
    is_initialized_ = std::exchange(other.is_initialized_, false);
  }

  return *this;
}

int App::run(const std::function<void()>& ui_callback) {
  if (!is_initialized_) {
    return 1;
  }

  ImGuiIO& imgui_io = ImGui::GetIO();
  bool running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // Forward event to ImGui
      ImGui_ImplSDL3_ProcessEvent(&event);

      // Handle close window
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window_)) {
        running = false;
      }
    }

    // When minimized, sleep a brief
    if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    // ImGui start from here
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // User UI from here
    ui_callback();

    // ImGui prepare draw for Render work
    ImGui::Render();
    // 6. Set OpenGL viewport matching current window display size
    glViewport(0, 0, static_cast<int>(imgui_io.DisplaySize.x),
               static_cast<int>(imgui_io.DisplaySize.y));

    // 7. Clear the background with a soft dark color
    glClearColor(0.85F, 0.85F, 0.85F, 0.85F);
    glClear(GL_COLOR_BUFFER_BIT);

    // 8. Send ImGui draw data to OpenGL
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 9. Swap the front and back buffers to display the rendered frame
    SDL_GL_SwapWindow(window_);
  }

  return 0;
}
