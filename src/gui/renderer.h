#include "rui.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <stdbool.h>

typedef enum {
  RUI_RENDERER_GLES2 = 1,
  RUI_RENDERER_GLES3
} rui_renderer_BackendType;

typedef struct {
  const char *name;
  int32_t width;
  int32_t height;
} rui_renderer_WindowProperties;

typedef struct {
  rui_renderer_BackendType backend_type;

  SDL_Window *window;
  rui_renderer_WindowProperties window_properties;

  SDL_GLContext context;
  GLuint program;
  GLuint vbo;
  GLuint ebo;
  GLint loc_projection;
  GLint loc_position;
  GLint loc_uv;
  GLint loc_color;

} rui_renderer_BackendData;

bool rui_renderer_init(rui_renderer_BackendData *bd);
void rui_renderer_draw(const rui_Context *rctx, rui_renderer_BackendData *bd);
void rui_renderer_destroy(rui_renderer_BackendData *bd);
