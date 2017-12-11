#include "ui.h"

void start_stockfish(int argc, char **argv);

int main()
{
#if INCLUDE_STOCKFISH
  start_stockfish(argc, argv);
#endif
  init_gl();
  load_assets();
  new_game();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(draw_board, 0, 0);
  emscripten_set_mousemove_callback(0, 0, 1, mouse_callback);
  emscripten_set_mousedown_callback(0, 0, 1, mouse_callback);
  return 0;
#else
#error Main loop implementation for other platforms
#endif
}
