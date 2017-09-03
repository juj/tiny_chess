#include <stdio.h>
#include <GLES2/gl2.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
extern "C" void upload_unicode_char_to_texture(int unicodeChar, int charSize, bool applyShadow);
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
#endif // ~__EMSCRIPTEN__

#include "board.h"

GLuint quad, colorPos, matPos;
GLuint textures[NUM_TEXTURES];
Board board;
int mouseHoverX = -1, mouseHoverY = -1;
int mouseSelectX = -1, mouseSelectY = -1;
bool uiNeedsRepaint = false;

void create_context()
{
#ifdef __EMSCRIPTEN__
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.alpha = 0;
  glContext = emscripten_webgl_create_context(0, &attrs);
  emscripten_webgl_make_context_current(glContext);
#else
#error Context creation not implemented for current platform!
#endif
}

GLuint compile_shader(GLenum shaderType, const char *src)
{
   GLuint shader = glCreateShader(shaderType);
   glShaderSource(shader, 1, &src, NULL);
   glCompileShader(shader);
   return shader;
}

GLuint create_program(GLuint vertexShader, GLuint fragmentShader)
{
   GLuint program = glCreateProgram();
   glAttachShader(program, vertexShader);
   glAttachShader(program, fragmentShader);
   glBindAttribLocation(program, 0, "pos");
   glLinkProgram(program);
   glUseProgram(program);
   return program;
}

void init_gl()
{
  create_context();

  static const char vertex_shader[] =
    "attribute vec4 pos;"
    "varying vec2 uv;"
    "uniform mat4 mat;"
    "void main(){"
      "uv=pos.xy;"
      "gl_Position=mat*pos;"
    "}";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);

  static const char fragment_shader[] =
    "precision lowp float;"
    "uniform sampler2D tex;"
    "varying vec2 uv;"
    "uniform vec4 color;"
    "void main(){"
      "gl_FragColor=color*texture2D(tex,uv);"
    "}";
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

  GLuint program = create_program(vs, fs);
  colorPos = glGetUniformLocation(program, "color");
  matPos = glGetUniformLocation(program, "mat");
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GLuint create_texture()
{
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture;
}

void load_assets()
{
  glGenBuffers(1, &quad);
  glBindBuffer(GL_ARRAY_BUFFER, quad);
  const float pos[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
  glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  textures[NO_UNIT] = create_texture();
  unsigned int whitePixel = 0xFFFFFFFF;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixel);

  const int charSize = 60;
  for(int i = WHITE_KING_TEXTURE; i < NUM_TEXTURES; ++i)
  {
    textures[i] = create_texture();
    const int UNICODE_WHITE_CHESS_KING = 0x2654;
    upload_unicode_char_to_texture(UNICODE_WHITE_CHESS_KING + (i-1), charSize, IS_TEXTURE_FOR_WHITE_PIECE(i));
  }
}

void draw(float x, float y, float w, float h, float r, float g, float b, float a, GLuint texture)
{
  float mat[16] = { w*2.f, 0, 0, 0, 0, h*2.f, 0, 0, 0, 0, 1, 0, x*2.f-1.f, y*2.f-1.f, 0, 1};
  glUniformMatrix4fv(matPos, 1, 0, mat);
  glUniform4f(colorPos, r, g, b, a);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

const float boardScale = 1.f / 8.f; // TODO: Make this scale dynamically to resize to full screen size

void draw_piece(float x, float y, int piece)
{
  int textureIndex = (IS_WHITE_PIECE(piece) ? SOLID_COLOR_TEXTURE : WHITE_PAWN_TEXTURE) + PIECE_TYPE(piece);
  const float verticalOffset = -1.f / 480.f;
  if (IS_WHITE_PIECE(piece)) draw(x * boardScale, y * boardScale + verticalOffset, boardScale, boardScale, 1.f, 1.f, 1.f, 1.f, textures[textureIndex]);
  else draw(x * boardScale, y * boardScale + verticalOffset, boardScale, boardScale, 0.f, 0.f, 0.f, 1.f, textures[textureIndex]);
}

void draw_solid(float x, float y, float r, float g, float b, float a)
{
  draw(x * boardScale, y * boardScale, boardScale, boardScale, r, g, b, a, textures[SOLID_COLOR_TEXTURE]);
}

void draw_background()
{
  for(int i = 0, y = 0; y < 8; ++y, ++i)
    for(int x = 0; x < 8; ++x, ++i)
    {
      if (i % 2 == 0) draw_solid(x, y, 0.463f, 0.589f, 0.337f, 1.f); // Black background
      else draw_solid(x, y, 0.933f, 0.933f, 0.834f, 1.f); // White background
    }
}

void draw_pieces()
{
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      if (board.board[y][x])
        draw_piece(x, y, board.board[y][x]);
}

void make_move(int srcX, int srcY, int dstX, int dstY)
{
  if (!board.is_valid_move(srcX, srcY, dstX, dstY)) return;
  board.make_move(srcX, srcY, dstX, dstY);
  mouseSelectX = -1;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *)
{
  int x = e->canvasX * 8 / 480;
  int y = (479 - e->canvasY) * 8 / 480;
  if (IS_OUT_OF_BOARD(x, y))
  {
    if (mouseHoverX != -1 || mouseHoverY != -1)
    {
      mouseHoverX = mouseHoverY = -1;
      uiNeedsRepaint = true;
    }
    return EM_FALSE;
  }
  switch(eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      if (x != mouseHoverX || y != mouseHoverY)
      {
        mouseHoverX = x, mouseHoverY = y;
        uiNeedsRepaint = true;
      }
      break;
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      if (mouseSelectX == -1)
      {
        if (PLAYER_COLOR(board.At(x, y)) == board.currentPlayer && board.has_valid_moves(x, y))
          mouseSelectX = x, mouseSelectY = y;
      }
      else if (mouseSelectX == x && mouseSelectY == y) mouseSelectX = -1;
      else make_move(mouseSelectX, mouseSelectY, x, y);
      uiNeedsRepaint = true;
      break;
  }
  return EM_FALSE;
}

void draw_board()
{
  if (!uiNeedsRepaint) return;
  glBindBuffer(GL_ARRAY_BUFFER, quad); // TODO: Remove this line, specified as a workaround to -s OFFSCREEN_FRAMEBUFFER=1 bug
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); // TODO: Remove this line, specified as a workaround to -s OFFSCREEN_FRAMEBUFFER=1 bug
  glEnableVertexAttribArray(0); // TODO: Remove this line, specified as a workaround to -s OFFSCREEN_FRAMEBUFFER=1 bug
  glDisable(GL_BLEND);
  draw_background();
  glEnable(GL_BLEND);
  // Draw a hint square if king is in check
  if (board.is_king_in_check(board.currentPlayer))
  {
    int kingX, kingY;
    board.find_king(board.currentPlayer == WHITE ? WHITE_KING : BLACK_KING, &kingX, &kingY);
    draw_solid(kingX, kingY, 1.f, 0.3f, 0.3f, 0.8f);
  }
  // If player has activate a piece for move, draw hint squares for legal positions to move to.
  if (mouseSelectX >= 0)
  {
    int moves[48*2];
    int *end = board.generate_moves(mouseSelectX, mouseSelectY, moves);
    for(int *m = moves; m != end; m += 2)
      draw_solid(m[0], m[1], 1.f, 0.8f, 0.6f, 0.8f);
  }
  if (mouseHoverX >= 0) draw_solid(mouseHoverX, mouseHoverY, 0.7f, 0.7f, 1.f, 0.6f); // Draw highlight square under mouse cursor
  if (mouseSelectX >= 0) draw_solid(mouseSelectX, mouseSelectY, 0.5f, 0.5f, 1.f, 1.f); // Draw even stronger highlight square under the selected piece
  draw_pieces();
  uiNeedsRepaint = false;
}

void new_game()
{
  board.new_game();
  uiNeedsRepaint = true;
}
