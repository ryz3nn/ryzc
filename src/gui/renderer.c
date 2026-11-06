#include "renderer.h"

#include <SDL3/SDL_opengles2.h>
#include <stdio.h>
#include <string.h>

#define MAX_QUADS 2048
#define MAX_VERTICES (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

// Just 12 bytes per vertex: 2 floats for pos, 4 bytes for RGBA
typedef struct {
  float x, y;
  uint8_t r, g, b, a;
} rui_Vertex;

static GLuint s_program = 0;
static GLuint s_vbo = 0;
static GLuint s_ebo = 0;

static GLint s_loc_pos = -1;
static GLint s_loc_color = -1;
static GLint s_loc_screen = -1;

static rui_Vertex s_vertices[MAX_VERTICES];
static uint32_t s_vertex_count = 0;

static const char *s_vertex_shader_src =
    "attribute vec2 a_pos;\n"
    "attribute vec4 a_color;\n"
    "uniform vec2 u_screen;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "    v_color = a_color / 255.0;\n"
    "    gl_Position = vec4(a_pos.x * (2.0 / u_screen.x) - 1.0,\n"
    "                       1.0 - a_pos.y * (2.0 / u_screen.y),\n"
    "                       0.0, 1.0);\n"
    "}\n";

static const char *s_fragment_shader_src = "precision mediump float;\n"
                                           "varying vec4 v_color;\n"
                                           "void main() {\n"
                                           "    gl_FragColor = v_color;\n"
                                           "}\n";

static GLuint compile_shader(GLenum type, const char *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    fprintf(stderr, "Shader compile error: %s\n", log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

bool rui_renderer_init(void) {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, s_vertex_shader_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, s_fragment_shader_src);
  if (!vs || !fs) {
    return false;
  }

  s_program = glCreateProgram();
  glAttachShader(s_program, vs);
  glAttachShader(s_program, fs);
  glLinkProgram(s_program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint ok = 0;
  glGetProgramiv(s_program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(s_program, sizeof(log), NULL, log);
    fprintf(stderr, "Program link error: %s\n", log);
    return false;
  }

  s_loc_pos = glGetAttribLocation(s_program, "a_pos");
  s_loc_color = glGetAttribLocation(s_program, "a_color");
  s_loc_screen = glGetUniformLocation(s_program, "u_screen");

  // Precompute static Index Buffer (0, 1, 2, 2, 3, 0)
  GLushort indices[MAX_INDICES];
  for (uint32_t i = 0; i < MAX_QUADS; i++) {
    indices[i * 6 + 0] = (GLushort)(i * 4 + 0);
    indices[i * 6 + 1] = (GLushort)(i * 4 + 1);
    indices[i * 6 + 2] = (GLushort)(i * 4 + 2);
    indices[i * 6 + 3] = (GLushort)(i * 4 + 2);
    indices[i * 6 + 4] = (GLushort)(i * 4 + 3);
    indices[i * 6 + 5] = (GLushort)(i * 4 + 0);
  }

  glGenBuffers(1, &s_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertices), NULL, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &s_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  return true;
}

void rui_renderer_render(const rui_Context *ctx) {
  if (!ctx || ctx->command_count == 0) {
    return;
  }

  s_vertex_count = 0;

  for (int32_t i = 0; i < ctx->command_count; i++) {
    const rui_Command *cmd = &ctx->commands[i];
    if (cmd->type != RUI_CMD_RECT) {
      continue;
    }

    if (s_vertex_count + 4 > MAX_VERTICES) {
      break;
    }

    float x = (float)cmd->rect.x;
    float y = (float)cmd->rect.y;
    float w = (float)cmd->rect.w;
    float h = (float)cmd->rect.h;
    rui_Color c = cmd->color;

    s_vertices[s_vertex_count + 0] =
        (rui_Vertex){.x = x, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    s_vertices[s_vertex_count + 1] = (rui_Vertex){
        .x = x + w, .y = y, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    s_vertices[s_vertex_count + 2] = (rui_Vertex){
        .x = x + w, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};
    s_vertices[s_vertex_count + 3] = (rui_Vertex){
        .x = x, .y = y + h, .r = c.r, .g = c.g, .b = c.b, .a = c.a};

    s_vertex_count += 4;
  }

  if (s_vertex_count == 0) {
    return;
  }

  // Upload dynamic quad vertices
  glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, s_vertex_count * sizeof(rui_Vertex),
                  s_vertices);

  // Blend state for alpha transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glUseProgram(s_program);
  glUniform2f(s_loc_screen, (float)ctx->screen_w, (float)ctx->screen_h);

  glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);

  glEnableVertexAttribArray(s_loc_pos);
  glVertexAttribPointer(s_loc_pos, 2, GL_FLOAT, GL_FALSE, sizeof(rui_Vertex),
                        (const void *)offsetof(rui_Vertex, x));

  glEnableVertexAttribArray(s_loc_color);
  glVertexAttribPointer(s_loc_color, 4, GL_UNSIGNED_BYTE, GL_FALSE,
                        sizeof(rui_Vertex),
                        (const void *)offsetof(rui_Vertex, r));

  uint32_t quad_count = s_vertex_count / 4;
  glDrawElements(GL_TRIANGLES, (GLsizei)(quad_count * 6), GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(s_loc_pos);
  glDisableVertexAttribArray(s_loc_color);
}

void rui_renderer_destroy(void) {
  if (s_vbo) {
    glDeleteBuffers(1, &s_vbo);
    s_vbo = 0;
  }
  if (s_ebo) {
    glDeleteBuffers(1, &s_ebo);
    s_ebo = 0;
  }
  if (s_program) {
    glDeleteProgram(s_program);
    s_program = 0;
  }
}
