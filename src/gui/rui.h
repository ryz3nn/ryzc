#pragma once
#include <stddef.h>
#include <stdint.h>

#define RUI_MAX_COMMANDS 1024
#define RUI_TEXT_SIZE 4096

typedef struct {
  float x, y, w, h;
} rui_Rect;

typedef struct {
  uint8_t r, g, b, a;
} rui_Color;

typedef struct {
  float x, y;
} rui_Vec2;

typedef enum { RUI_CMD_RECT = 1, RUI_CMD_TEXT } rui_CommandType;

typedef struct {
  rui_CommandType type;
  // Data required to draw
  rui_Rect rect;
  rui_Color color;
} rui_Command;

typedef struct {
  int32_t screen_w;
  int32_t screen_h;

  rui_Command commands[RUI_MAX_COMMANDS];
  int32_t command_count;
  uint32_t command_dropped;
} rui_Context;

rui_Rect rui_create_rect(float x, float y, float w, float h);
rui_Color rui_create_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
rui_Vec2 rui_create_vec2(float x, float y);

void rui_init(rui_Context *ctx);
void rui_begin(rui_Context *ctx, int32_t screen_w, int32_t screen_h);
void rui_end(rui_Context *ctx);

void rui_draw_rect(rui_Context *ctx, rui_Rect rect, rui_Color color);
void rui_draw_text(rui_Context *ctx, rui_Rect rect, rui_Color color);
