#include "rui.h"

#include <stdio.h>
#include <string.h>

void rui_init(rui_Context *ctx) { memset(ctx, 0, sizeof(*ctx)); }

void rui_begin(rui_Context *ctx, int32_t screen_w, int32_t screen_h) {
  ctx->screen_w = screen_w;
  ctx->screen_h = screen_h;
  ctx->command_count = 0;
}

void rui_end(rui_Context *ctx) {
  if (ctx->command_dropped > 0) {
    fprintf(stderr, "rui warning: Dropped %d commands\n", ctx->command_dropped);
  }
}

rui_Rect rui_create_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
  rui_Rect rect = {.x = x, .y = y, .w = w, .h = h};
  return rect;
}

rui_Color rui_create_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  rui_Color color = {.r = r, .g = g, .b = b, .a = a};
  return color;
}

static rui_Command *rctx_push_cmd(rui_Context *ctx, rui_CommandType type) {
  if (ctx->command_count >= RUI_MAX_COMMANDS) {
    ctx->command_dropped++;
    return NULL;
  }
  rui_Command *cmd = &ctx->commands[ctx->command_count++];
  cmd->type = type;
  return cmd;
}

void rui_draw_rect(rui_Context *ctx, rui_Rect rect, rui_Color color) {
  if (rect.w == 0 || rect.h == 0 || color.a == 0)
    return;
  rui_Command *cmd = rctx_push_cmd(ctx, RUI_CMD_RECT);
  if (cmd != NULL) {
    cmd->rect = rect;
    cmd->color = color;
  }
}
