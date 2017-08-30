#pragma once

void init_gl();
void load_assets();
void init_board();
void update();

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData);
#endif
