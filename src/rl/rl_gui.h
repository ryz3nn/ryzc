#ifndef INCLUDE_RL_GUI_H
#define INCLUDE_RL_GUI_H

/// Quick test
// #define RL_GUI_IMPL
// #define RL_GUI_PLATFORM_SDL3
// #define RL_GUI_BACKEND_GLES2

/// Vendoring
#ifdef RL_GUI_PLATFORM_SDL3
#include <SDL3/SDL.h>
#endif
#ifdef RL_GUI_BACKEND_GLES2
#include <GLES2/gl2.h>
#endif

/// Header
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// Define
#define MAX_COMMANDS 1024
#define MAX_QUADS 2048 // Max: 16384
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

#endif // INCLUDE_RL_GUI_H

/// Implementaion
#ifdef RL_GUI_IMPL

typedef enum {
  RLGUI_STATUS_OK = 0,
  RLGUI_STATUS_ERR = 1,
} rlgui_Status;

// Device
typedef struct {
  SDL_Window *window;
  SDL_GLContext context;

  GLuint program;
  GLuint vbo;
  GLuint ebo;
  GLint loc_projection;
  GLint loc_position;
  GLint loc_color;

} rlgui_DeviceData;

// Context
typedef struct {
  float x, y;
} rlgui_Vec2;

typedef struct {
  float x, y, w, h;
} rlgui_Rect;

typedef struct {
  uint8_t r, g, b, a;
} rlgui_Color;

typedef struct {
  float x, y;
  uint8_t r, g, b, a;
} rlgui_Vertex;

typedef enum {
  RLGUI_COMMAND_RECT = 1,
} rlgui_CommandType;

typedef struct {
  rlgui_CommandType type;
  // Data required to draw
  rlgui_Rect rect;
  rlgui_Color color;
} rlgui_Command;

typedef struct {
  int screen_w;
  int screen_h;

  rlgui_Command commands[MAX_COMMANDS];
  int command_count;
} rlgui_Context;

// Public API
static int rlgui_init(rlgui_DeviceData *dd, const char *title, int width, int height);
static void rlgui_render(rlgui_DeviceData *dd, rlgui_Context *ctx);
static void rlgui_destroy(rlgui_DeviceData *dd);

static void rlgui_begin(rlgui_Context *ctx, int width, int height);
static void rlgui_end(rlgui_Context *ctx);

// Backend
static GLuint rlgui__load_shader(GLenum type, const char *src) {
  GLint compiled;
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    fprintf(stderr, "Load shader failed\n");
    return 0;
  }

  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    char log[512];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    fprintf(stderr, "Compile shader failed: %s\n", log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static int rlgui_init(rlgui_DeviceData *dd, const char *title, int width, int height) {
  SDL_Window *window;
  SDL_GLContext context;

  // Platform init: SDL3
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Init SDL failed: %s\n", SDL_GetError());
    return RLGUI_STATUS_ERR;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL create window failed: %s\n", SDL_GetError());
    return RLGUI_STATUS_ERR;
  }

  context = SDL_GL_CreateContext(window);
  if (!context) {
    fprintf(stderr, "SDL create opengl context failed: %s\n", SDL_GetError());
    return RLGUI_STATUS_ERR;
  }

  SDL_GL_SetSwapInterval(1);
  dd->window = window;
  dd->context = context;

  // Rendering Backend: OpenGL ES 2.0
  GLuint program;
  GLint linked;

  const char *vertex_src = "uniform mat4 u_projection;\n"
                           "attribute vec2 a_position;\n"
                           "attribute vec4 a_color;\n"
                           "varying vec4 v_color;\n"
                           "void main()\n"
                           "{\n"
                           "gl_Position = u_projection * vec4(a_position.xy, 0.0 , 1.0);\n"
                           "v_color = a_color;\n"
                           "}\n";

  const char *fragment_src = "precision mediump float;\n"
                             "varying vec4 v_color;\n"
                             "void main()\n"
                             "{\n"
                             "    gl_FragColor = v_color;\n"
                             "}\n";

  GLuint vs = rlgui__load_shader(GL_VERTEX_SHADER, vertex_src);
  GLuint fs = rlgui__load_shader(GL_FRAGMENT_SHADER, fragment_src);
  if (!vs || !fs) {
    return RLGUI_STATUS_ERR;
  }

  program = glCreateProgram();
  if (program == 0) {
    fprintf(stderr, "Create program failed:\n");
    return RLGUI_STATUS_ERR;
  }
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    char log[512];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    fprintf(stderr, "Link program failed: %s\n", log);
    glDeleteProgram(program);
    return RLGUI_STATUS_ERR;
  }
  dd->program = program;
  glGenBuffers(1, &dd->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, dd->vbo);

  // Precompute static Index Buffer
  GLushort indices[MAX_INDICES];
  for (int i = 0; i < MAX_QUADS; i++) {
    indices[i * 6 + 0] = (GLushort)(i * 4 + 0);
    indices[i * 6 + 1] = (GLushort)(i * 4 + 1);
    indices[i * 6 + 2] = (GLushort)(i * 4 + 2);
    indices[i * 6 + 3] = (GLushort)(i * 4 + 2);
    indices[i * 6 + 4] = (GLushort)(i * 4 + 3);
    indices[i * 6 + 5] = (GLushort)(i * 4 + 0);
  }
  glGenBuffers(1, &dd->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dd->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  dd->loc_projection = glGetUniformLocation(dd->program, "u_projection");
  dd->loc_position = (GLuint)glGetAttribLocation(dd->program, "a_position");
  dd->loc_color = (GLuint)glGetAttribLocation(dd->program, "a_color");

  return RLGUI_STATUS_OK;
}

static rlgui_Vertex vertices_array[MAX_VERTICES];
static void rlgui_render(rlgui_DeviceData *dd, rlgui_Context *ctx) {
  // Empty command list -> exit
  if (!ctx || ctx->command_count == 0) {
    return;
  }
  // Loop through command list
  int vertex_count = 0;
  for (int i = 0; i < ctx->command_count; i++) {
    const rlgui_Command *cmd = &ctx->commands[i];
    if (vertex_count + 4 > MAX_VERTICES) {
      break;
    }

    float x = cmd->rect.x;
    float y = cmd->rect.y;
    float w = cmd->rect.w;
    float h = cmd->rect.h;
    rlgui_Color c = cmd->color;

    vertices_array[vertex_count + 0] =
        (rlgui_Vertex){.x = x, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 1] =
        (rlgui_Vertex){.x = x + w, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 2] =
        (rlgui_Vertex){.x = x + w, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 3] =
        (rlgui_Vertex){.x = x, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};

    vertex_count += 4;
  }
  if (vertex_count == 0) {
    return;
  }

  // Render State: alpha, blending, ...
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  // Viewport and project matrix
  glViewport(0, 0, ctx->screen_w, ctx->screen_h);
  glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  // clang-format off
  const float projection_matrix[16] = {
      2.0f / ctx->screen_w,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      -2.0f / ctx->screen_h,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      -1.0f,
      0.0f,
      -1.0f,
      1.0f,
      0.0f,
      1.0f,
  };
  glUseProgram(dd->program);
  glUniformMatrix4fv(dd->loc_projection, 1, GL_FALSE, &projection_matrix[0]);
  // Update VBO, EBO
  glBindBuffer(GL_ARRAY_BUFFER, dd->vbo);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(rlgui_Vertex), vertices_array,
               GL_STREAM_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dd->ebo);
  glEnableVertexAttribArray(dd->loc_position);
  glVertexAttribPointer(dd->loc_position, 2, GL_FLOAT, GL_FALSE, sizeof(rlgui_Vertex),
                        (const void *)offsetof(rlgui_Vertex, x));
  // glEnableVertexAttribArray(dd->loc_uv);
  // glVertexAttribPointer(dd->loc_uv, 2, GL_FLOAT, GL_FALSE,
  // sizeof(rlgui_Vertex),
  //                       (const void *)offsetof(rlgui_Vertex, u));
  glEnableVertexAttribArray(dd->loc_color);
  glVertexAttribPointer(dd->loc_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rlgui_Vertex),
                        (const void *)offsetof(rlgui_Vertex, r));
  int quad_count = vertex_count / 4;
  glDrawElements(GL_TRIANGLES, (GLsizei)quad_count * 6, GL_UNSIGNED_SHORT, 0);

  // Clean: disable, unbind, ....
  glDisableVertexAttribArray(dd->loc_position);
  // glDisableVertexAttribArray(dd->loc_uv);
  glDisableVertexAttribArray(dd->loc_color);
}

static void rlgui_destroy(rlgui_DeviceData *dd) {
  glDeleteBuffers(1, &dd->ebo);
  glDeleteBuffers(1, &dd->vbo);
  glDeleteProgram(dd->program);
  SDL_GL_DestroyContext(dd->context);
  SDL_DestroyWindow(dd->window);
  SDL_Quit();
}

// Frontend
static rlgui_Rect rlgui_create_rect(float x, float y, float w, float h) {
  return (rlgui_Rect){.x = x, .y = y, .w = w, .h = h};
}

static rlgui_Color rlgui_create_color(uint8_t r, uint8_t g,uint8_t b,uint8_t a) {
  return (rlgui_Color){.r = r, .g = g, .b = b, .a = a};
}

static void rlgui_begin(rlgui_Context *ctx, int width, int height) {
  ctx->screen_w = width;
  ctx->screen_h = height;
  ctx->command_count = 0;
}

static rlgui_Command *rlgui__ctx_push_cmd(rlgui_Context *ctx, rlgui_CommandType type) {
  if (ctx->command_count >= MAX_COMMANDS) {
    return NULL;
  }
  rlgui_Command *cmd = &ctx->commands[ctx->command_count++];
  cmd->type = type;
  return cmd;
}
static void rlgui_draw_rect(rlgui_Context *ctx, rlgui_Rect rect, rlgui_Color color) {
  if (rect.w == 0 || rect.h == 0 || color.a == 0) {
    return;
  }
  rlgui_Command *cmd = rlgui__ctx_push_cmd(ctx, RLGUI_COMMAND_RECT);
  if (cmd != NULL) {
    cmd->rect = rect;
    cmd->color = color;
  }
}

#endif // RL_GUI_IMPL
