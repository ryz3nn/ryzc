#pragma once
#include "renderer.h"

#include <GLES2/gl2.h>
#include <stdio.h>

#define MAX_QUADS 2048 // Max: 16384
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)


typedef struct {
  float x, y;
  float u, v;
  uint8_t r, g, b, a;
} rui_Vertex;

rui_Vertex vertices_array[MAX_VERTICES];
static bool init_sdl_gles2(rui_renderer_BackendData *bd) {
  SDL_Window *window;
  SDL_GLContext context;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Init SDL failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow(
      bd->window_properties.name, bd->window_properties.width,
      bd->window_properties.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL create window failed: %s\n", SDL_GetError());
    return false;
  }

  context = SDL_GL_CreateContext(window);
  if (!context) {
    fprintf(stderr, "SDL create opengl context failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetSwapInterval(1);

  bd->window = window;
  bd->context = context;
  return true;
}

GLuint load_shader(GLenum type, const char *src) {
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

bool create_device_object(rui_renderer_BackendData *bd) {
  GLuint program;
  GLint linked;

  const char *vertex_src =
      "uniform mat4 u_projection;\n"
      "attribute vec2 a_position;\n"
      "attribute vec2 a_uv;\n"
      "attribute vec4 a_color;\n"
      "varying vec2 v_uv;\n"
      "varying vec4 v_color;\n"
      "void main()\n"
      "{\n"
      "gl_Position = u_projection * vec4(a_position.xy, 0.0 , 1.0);\n"
      "v_uv = a_uv;\n"
      "v_color = a_color;\n"
      "}\n";

  const char *fragment_src = "precision mediump float;\n"
                             "varying vec2 v_uv;\n"
                             "varying vec4 v_color;\n"
                             "void main()\n"
                             "{\n"
                             "    gl_FragColor = v_color;\n"
                             "}\n";

  GLuint vs = load_shader(GL_VERTEX_SHADER, vertex_src);
  GLuint fs = load_shader(GL_FRAGMENT_SHADER, fragment_src);
  if (!vs || !fs) {
    return false;
  }

  program = glCreateProgram();
  if (program == 0) {
    fprintf(stderr, "Create program failed:\n");
    return false;
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
    return false;
  }
  bd->program = program;
  glGenBuffers(1, &bd->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, bd->vbo);

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
  glGenBuffers(1, &bd->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  bd->loc_projection = glGetUniformLocation(bd->program, "u_projection");
  bd->loc_position = (GLuint)glGetAttribLocation(bd->program, "a_position");
  bd->loc_uv = (GLuint)glGetAttribLocation(bd->program, "a_uv");
  bd->loc_color = (GLuint)glGetAttribLocation(bd->program, "a_color");

  return true;
}

bool rui_renderer_init(rui_renderer_BackendData *bd) {
  switch (bd->backend_type) {
  case RUI_RENDERER_GLES2:
     if(!init_sdl_gles2(bd)) {
       return false;
     };
     break;
  case RUI_RENDERER_GLES3:
    return false;
  }

  if (!create_device_object(bd)) {
    return false;
  }

  return true;
}

void rui_renderer_draw(const rui_Context *rctx, rui_renderer_BackendData *bd) {
  // Empty command list -> exit
  if (!rctx || rctx->command_count == 0) {
    return;
  }
  // Loop through command list
  int vertex_count = 0;
  for (int i = 0; i < rctx->command_count; i++) {
    const rui_Command *cmd = &rctx->commands[i];
    if (cmd->type != RUI_CMD_RECT) {
      continue;
    }
    if (vertex_count + 4 > MAX_VERTICES) {
      break;
    }

    float x = cmd->rect.x;
    float y = cmd->rect.y;
    float w = cmd->rect.w;
    float h = cmd->rect.h;
    rui_Color c = cmd->color;

    vertices_array[vertex_count + 0] =
        (rui_Vertex){.x = x, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 1] = (rui_Vertex){
        .x = x + w, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 2] = (rui_Vertex){
        .x = x + w, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    vertices_array[vertex_count + 3] = (rui_Vertex){
        .x = x, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};

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
  glViewport(0, 0, rctx->screen_w, rctx->screen_h);
  glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  const float projection_matrix[16] = {
    2.0f / rctx->screen_w,  0.0f,                   0.0f,  0.0f,  
    0.0f,                  -2.0f / rctx->screen_h,  0.0f,  0.0f, 
    0.0f,                   0.0f,                  -1.0f,  0.0f,
   -1.0f,                   1.0f,                   0.0f,  1.0f,};
  glUseProgram(bd->program);
  glUniformMatrix4fv(bd->loc_projection, 1, GL_FALSE, &projection_matrix[0]);
  // Update VBO, EBO
  glBindBuffer(GL_ARRAY_BUFFER, bd->vbo);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(rui_Vertex), vertices_array,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ebo);
  glEnableVertexAttribArray(bd->loc_position);
  glVertexAttribPointer(bd->loc_position, 2, GL_FLOAT, GL_FALSE,
                        sizeof(rui_Vertex),
                        (const void *)offsetof(rui_Vertex, x));
  glEnableVertexAttribArray(bd->loc_uv);
  glVertexAttribPointer(bd->loc_uv, 2, GL_FLOAT, GL_FALSE, sizeof(rui_Vertex),
                        (const void *)offsetof(rui_Vertex, u));
  glEnableVertexAttribArray(bd->loc_color);
  glVertexAttribPointer(bd->loc_color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                        sizeof(rui_Vertex),
                        (const void *)offsetof(rui_Vertex, r));
  int quad_count = vertex_count / 4;
  glDrawElements(GL_TRIANGLES, (GLsizei)quad_count * 6, GL_UNSIGNED_SHORT, 0);

  // Clean: disable, unbind, ....
  glDisableVertexAttribArray(bd->loc_position);
  glDisableVertexAttribArray(bd->loc_uv);
  glDisableVertexAttribArray(bd->loc_color);
}

void rui_renderer_destroy(rui_renderer_BackendData *bd) {
  glDeleteBuffers(1, &bd->vbo);
  glDeleteBuffers(1, &bd->ebo);
  glDeleteProgram(bd->program);
  switch (bd->backend_type) {
  case RUI_RENDERER_GLES2:
    SDL_GL_DestroyContext(bd->context);
    SDL_DestroyWindow(bd->window);
    SDL_Quit();
    break;
  case RUI_RENDERER_GLES3:
    break;
  }
}
