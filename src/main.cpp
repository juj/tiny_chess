#include <stdio.h>
#include <gles2/gl2.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
#endif

#include "board.h"

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
   glBindAttribLocation(program, 0, "in_pos");
   glBindAttribLocation(program, 1, "in_uv");
   glLinkProgram(program);
   glUseProgram(program);
   return program;
}

GLuint quad, colorPos, matPos;

void init_gl()
{
  create_context();

  static const char vertex_shader[] =
    "attribute vec4 in_pos;\n"
    "varying vec2 uv;\n"
    "uniform mat4 mat;\n"
    "void main(void) {\n"
    "  uv = in_pos.xy;\n"
    "  gl_Position = mat*vec4(in_pos.xy,0,1);\n"
    "}";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);

  static const char fragment_shader[] =
    "precision lowp float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 uv;\n"
    "uniform vec4 color;\n"
    "void main(void) {\n"
    "  gl_FragColor = color*texture2D(tex, uv);\n"
    "}";
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

  GLuint program = create_program(vs, fs);

  colorPos = glGetUniformLocation(program, "color");
  matPos = glGetUniformLocation(program, "mat");

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#define UNICODE_WHITE_CHESS_KING 0x2654

GLuint textures[NUM_TEXTURES];

extern "C" void upload_unicode_char_to_texture(int unicodeChar, int charSize, bool applyShadow);

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

const float boardScale = 1.f / 8.f;

void draw_piece(float x, float y, int piece)
{
  int textureIndex = (IS_WHITE_PIECE(piece) ? SOLID_COLOR_TEXTURE : WHITE_PAWN_TEXTURE) + PIECE_TYPE(piece);
#define VERTICAL_OFFSET (-1.f / 480.f)
  if (IS_WHITE_PIECE(piece)) draw(x * boardScale, y * boardScale + VERTICAL_OFFSET, boardScale, boardScale, 1.f, 1.f, 1.f, 1.f, textures[textureIndex]);
  else draw(x * boardScale, y * boardScale + VERTICAL_OFFSET, boardScale, boardScale, 0.f, 0.f, 0.f, 1.f, textures[textureIndex]);
}

void draw_solid(float x, float y, float r, float g, float b, float a)
{
  draw(x * boardScale, y * boardScale, boardScale, boardScale, r, g, b, a, textures[SOLID_COLOR_TEXTURE]);
}

void draw_background()
{
#define BLACK_BG 0.463f, 0.589f, 0.337f, 1.f
#define WHITE_BG 0.933f, 0.933f, 0.834f, 1.f

  int i = 0;
  for(int y = 0; y < 8; ++y)
  {
    for(int x = 0; x < 8; ++x)
    {
      if (i%2 == 0) draw_solid(x, y, BLACK_BG);
      else draw_solid(x, y, WHITE_BG);
      ++i;
    }
    ++i;
  }
}

void draw_pieces(const Board &board)
{
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
    {
      if (board.At(x, y)) draw_piece(x, y, board.At(x, y));
    }
}

Board board;

int mouseHoverX = -1, mouseHoverY = -1;
int mouseSelectX = -1, mouseSelectY = -1;

bool has_valid_moves(int x, int y)
{
  int moves[48*2];
  int num = board.generate_moves(x, y, moves);
  return num != 0;
}

bool is_valid_move(int srcX, int srcY, int dstX, int dstY)
{
  int moves[48*2];
  int end = board.generate_moves(srcX, srcY, moves);
  for(int *m = moves, *e = moves + end; m != e; m += 2)
  {
    if (m[0] == dstX && m[1] == dstY)
      return true;
  }
  return false;
}

void make_move(int srcX, int srcY, int dstX, int dstY)
{
  if (!is_valid_move(srcX, srcY, dstX, dstY)) return;

  board.make_move(srcX, srcY, dstX, dstY);

  mouseSelectX = -1;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
  int x = e->canvasX * 8 / 480;
  int y = (479 - e->canvasY) * 8 / 480;        
  switch(eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      mouseHoverX = x;
      mouseHoverY = y;
      break;
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      if (mouseSelectX == -1)
      {
        if (PLAYER_COLOR(board.At(x, y)) == board.currentPlayer && has_valid_moves(x, y))
        {
          mouseSelectX = x;
          mouseSelectY = y;
        }
      }
      else if (mouseSelectX == x && mouseSelectY == y)
      {
        mouseSelectX = -1;
      }
      else
      {
        make_move(mouseSelectX, mouseSelectY, x, y);
      }
      break;
  }
  return EM_FALSE;
}

void render()
{
  glClearColor(1,1,0,1);
  glClear(GL_COLOR_BUFFER_BIT);

  glDisable(GL_BLEND);

  draw_background();

  glEnable(GL_BLEND);

  if (board.is_king_in_check(board.currentPlayer))
  {
    int kingX, kingY;
    board.find_first_piece(board.currentPlayer == WHITE ? WHITE_KING : BLACK_KING, &kingX, &kingY);
#define KING_IN_CHECK_HINT_COLOR 1.f, 0.3f, 0.3f, 0.8f
    draw_solid(kingX, kingY, KING_IN_CHECK_HINT_COLOR);
  }

#define VALID_MOVE_HINT_COLOR 1.f, 0.8f, 0.6f, 0.8f

  if (mouseSelectX >= 0)
  {
    int moves[48*2];
    int end = board.generate_moves(mouseSelectX, mouseSelectY, moves);
    for(int *m = moves, *e = moves + end; m != e; m += 2)
    {
      draw_solid(m[0], m[1], VALID_MOVE_HINT_COLOR);
    }
  }

#define MOUSE_HOVER_BG 0.7f, 0.7f, 1.f, 0.6f

  if (mouseHoverX >= 0)
    draw_solid(mouseHoverX, mouseHoverY, MOUSE_HOVER_BG);

#define MOUSE_SELECT_BG 0.5f, 0.5f, 1.f, 1.f

  if (mouseSelectX >= 0)
    draw_solid(mouseSelectX, mouseSelectY, MOUSE_SELECT_BG);

  draw_pieces(board);
}

void update()
{

  // if (needsRepaint)
  render();
}


int main()
{
  init_gl();
  load_assets();
  board.init();
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(update, 0, 0);
  emscripten_set_mousemove_callback(0, 0, 1, mouse_callback);
  emscripten_set_mousedown_callback(0, 0, 1, mouse_callback);
#else
  for(;;) update();
#endif
}
