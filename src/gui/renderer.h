#pragma once
#include "rui.h"

#include <stdbool.h>

bool rui_renderer_init(void);
void rui_renderer_render(const rui_Context *ctx);
void rui_renderer_destroy(void);
