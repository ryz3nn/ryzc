#include <GLES2/gl2.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>

#include <SDL3/SDL_video.h>
#include <stdint.h>
#include <stdio.h>

static const char *vertex_src = "attribute vec3 a_position;\n"
                                "void main() {\n"
                                "    gl_Position = vec4(a_position, 1.0);\n"
                                "}\n";

static const char *fragment_src =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.3, 0.2, 1.0);\n"
    "}\n";

typedef struct {
  SDL_Window *window;
  SDL_GLContext context;
  GLuint vbo;
  GLuint program;
} UserData;

bool init(UserData *);
void draw(UserData *);
void destroy(UserData *);

UserData create_user_data(const char *, const char *);
GLuint load_shader(GLenum, const char *);
GLuint create_vbo(void);

bool init(UserData *user_data) {
  SDL_Window *window;
  SDL_GLContext context;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL Init failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow("ryzc", 800, 600,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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

  user_data->window = window;
  user_data->context = context;
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

GLuint create_program(const char *vertex_src, const char *fragment_src) {
  GLuint program;
  GLint linked;

  GLuint vs = load_shader(GL_VERTEX_SHADER, vertex_src);
  GLuint fs = load_shader(GL_FRAGMENT_SHADER, fragment_src);
  if (!vs || !fs) {
    return 0;
  }

  program = glCreateProgram();
  if (program == 0) {
    fprintf(stderr, "Create program failed:\n");
    return 0;
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
    return 0;
  }
  return program;
}

GLuint create_vbo(void) {
  float vertices[] = {
      -0.5, -0.5, 0.0, 0.5, -0.5, 0.0, 0.0, 0.5, 0.0,
  };
  GLuint vbo;

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  return vbo;
}

void draw(UserData *user_data) {
  GLint position = glGetAttribLocation(user_data->program, "a_position");
  glUseProgram(user_data->program);
  glBindBuffer(GL_ARRAY_BUFFER, user_data->vbo);

  glEnableVertexAttribArray(position);
  glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisableVertexAttribArray(position);
}

void destroy(UserData *user_data) {
  glDeleteBuffers(1, &user_data->vbo);
  glDeleteProgram(user_data->program);
  SDL_GL_DestroyContext(user_data->context);
  SDL_DestroyWindow(user_data->window);
  SDL_Quit();
}

int main(void) {
  bool running = true;
  UserData user_data;
  if (!init(&user_data)) {
    return 0;
  }

  user_data.program = create_program(vertex_src, fragment_src);
  user_data.vbo = create_vbo();

  while (running) {
    int w, h;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }
    SDL_GetWindowSizeInPixels(user_data.window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    draw(&user_data);
    SDL_GL_SwapWindow(user_data.window);
  }

  destroy(&user_data);
  return 0;
}
