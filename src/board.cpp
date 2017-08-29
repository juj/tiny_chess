#include "board.h"

#define IS_WHITE_AT(x,y) (IS_WHITE_PIECE(At((x),(y))))
#define IS_BLACK_AT(x,y) (IS_BLACK_PIECE(At((x),(y))))

#define IS_EMPTY_OR_OPPONENT_AT(player_color, x, y) IS_EMPTY_OR_OPPONENT(player_color, At((x), (y)))
#define IS_OPPONENT_AT(player_color, x, y) IS_OPPONENT(player_color, At((x), (y)))

#define IS_OUT_OF_BOARD(x, y) ((x) < 0 || (x) >= 8 || (y) < 0 || (y) >= 8)

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define ABS(x) ((x) >= 0 ? (x) : -(x))

void Board::init()
{
  out = OUT_OF_BOARD;
  currentPlayer = WHITE;
  enpassantX = enpassantY = -1;
  castlingPiecesAtHome[0] = castlingPiecesAtHome[1] = KING_AT_HOME | KING_ROOK_AT_HOME | QUEEN_ROOK_AT_HOME;

  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      At(x, y) = 0;

  At(0, 0) = WHITE_ROOK;
  At(1, 0) = WHITE_KNIGHT;
  At(2, 0) = WHITE_BISHOP;
  At(3, 0) = WHITE_QUEEN;
  At(4, 0) = WHITE_KING;
  At(5, 0) = WHITE_BISHOP;
  At(6, 0) = WHITE_KNIGHT;
  At(7, 0) = WHITE_ROOK;

  At(0, 7) = BLACK_ROOK;
  At(1, 7) = BLACK_KNIGHT;
  At(2, 7) = BLACK_BISHOP;
  At(3, 7) = BLACK_QUEEN;
  At(4, 7) = BLACK_KING;
  At(5, 7) = BLACK_BISHOP;
  At(6, 7) = BLACK_KNIGHT;
  At(7, 7) = BLACK_ROOK;

  for(int i = 0; i < 8; ++i)
  {
    At(i, 1) = WHITE_PAWN;
    At(i, 6) = BLACK_PAWN;
  }
}

void Board::make_move(int srcX, int srcY, int dstX, int dstY)
{
  int myColor = PLAYER_COLOR(At(srcX, srcY));
  At(dstX, dstY) = At(srcX, srcY);
  At(srcX, srcY) = 0;

  // Did we make an en passant capture?
  if (dstX == enpassantX && dstY == enpassantY && PIECE_TYPE(At(dstX, dstY)) == PAWN)
  {
    At(dstX, srcY) = 0;
  }

  // Set or clear en passant state
  if (PIECE_TYPE(At(dstX, dstY)) == PAWN && ABS(dstY-srcY) == 2)
  {
    enpassantX = srcX;
    enpassantY = (srcY + dstY) / 2;
  }
  else
    enpassantX = enpassantY = -1;

  // Auto-promote to queen. (TODO: Add UI dialog to ask what to promote to)
  if (PIECE_TYPE(At(dstX, dstY)) == PAWN && PLAYER_COLOR(At(dstX, dstY)) == WHITE && dstY == 7) At(dstX, dstY) = WHITE_QUEEN;
  else if (PIECE_TYPE(At(dstX, dstY)) == PAWN && PLAYER_COLOR(At(dstX, dstY)) == BLACK && dstY == 0) At(dstX, dstY) = BLACK_QUEEN;

  // Castling?
  int castlingSide = (myColor == WHITE) ? 0 : 1;
  if (PIECE_TYPE(At(dstX, dstY)) == KING)
  {
    if (ABS(dstX-srcX) > 1)
    {
      // Moving the king was already handed in the beginning of this function. Handle the rook here:
      if (dstX > srcX) // Kingside
      {
        At(5, dstY) = At(7, dstY);
        At(7, dstY) = 0;
      }
      else // Queenside
      {
        At(3, dstY) = At(0, dstY);
        At(0, dstY) = 0;
      }
    }
    castlingPiecesAtHome[castlingSide] &= ~KING_AT_HOME;
  }

  // One of the rooks moved that could have castled?
  if (PIECE_TYPE(At(dstX, dstY)) == ROOK)
  {
    int castlingRank = (myColor == WHITE) ? 0 : 7;
    if (srcX == 0 && srcY == castlingRank) castlingPiecesAtHome[castlingSide] &= ~QUEEN_ROOK_AT_HOME;
    else if (srcX == 7 && srcY == castlingRank) castlingPiecesAtHome[castlingSide] &= ~KING_ROOK_AT_HOME;
  }

  currentPlayer = OPPONENT_COLOR(myColor);
}

bool Board::is_king_in_check(int color)
{
  int opponentControlledSquares[8][8];
  mark_controlled_squares(OPPONENT_COLOR(color), opponentControlledSquares);

  int kingX, kingY;
  find_first_piece(color == WHITE ? WHITE_KING : BLACK_KING, &kingX, &kingY);
  return opponentControlledSquares[kingY][kingX] != 0;
}

#define APPEND_MOVE(x, y) do { *moves++ = x; *moves++ = y; } while(0)
#define MARK_CONTROLLED(x, y) squares[(y)][(x)] = 1

int Board::generate_pawn_moves(int x, int y, int *moves)
{
  int *start = moves;
  int myColor = PLAYER_COLOR(At(x, y));
  int dir = (myColor == WHITE) ? 1 : -1;
  if (!At(x, y+dir))
  {
    const int homeRow = (myColor == WHITE) ? 1 : 6;
    APPEND_MOVE(x, y+dir);
    if (y == homeRow && !At(x, y+dir+dir)) APPEND_MOVE(x, y+dir+dir);
  }
  if (IS_OPPONENT_AT(myColor, x-1, y+dir) || (x-1 == enpassantX && y+dir == enpassantY)) APPEND_MOVE(x-1, y+dir);
  if (IS_OPPONENT_AT(myColor, x+1, y+dir) || (x+1 == enpassantX && y+dir == enpassantY)) APPEND_MOVE(x+1, y+dir);
  return moves - start;
}

void Board::mark_pawn_controlled_squares(int x, int y, int squares[8][8])
{
  int dir = (PLAYER_COLOR(At(x, y)) == WHITE) ? 1 : -1;
  if (!IS_OUT_OF_BOARD(x-1, y+dir)) MARK_CONTROLLED(x-1, y+dir);
  if (!IS_OUT_OF_BOARD(x+1, y+dir)) MARK_CONTROLLED(x+1, y+dir);
}

int Board::generate_knight_moves(int x, int y, int *moves)
{
  int *start = moves;
  int myColor = PLAYER_COLOR(At(x, y));
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-2, y-1)) APPEND_MOVE(x-2, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-2, y+1)) APPEND_MOVE(x-2, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+2, y-1)) APPEND_MOVE(x+2, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+2, y+1)) APPEND_MOVE(x+2, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-1, y-2)) APPEND_MOVE(x-1, y-2);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-1, y+2)) APPEND_MOVE(x-1, y+2);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+1, y-2)) APPEND_MOVE(x+1, y-2);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+1, y+2)) APPEND_MOVE(x+1, y+2);
  return moves - start;
}

void Board::mark_knight_controlled_squares(int x, int y, int squares[8][8])
{
  if (!IS_OUT_OF_BOARD(x-2, y-1)) MARK_CONTROLLED(x-2, y-1);
  if (!IS_OUT_OF_BOARD(x-2, y+1)) MARK_CONTROLLED(x-2, y+1);
  if (!IS_OUT_OF_BOARD(x+2, y-1)) MARK_CONTROLLED(x+2, y-1);
  if (!IS_OUT_OF_BOARD(x+2, y+1)) MARK_CONTROLLED(x+2, y+1);
  if (!IS_OUT_OF_BOARD(x-1, y-2)) MARK_CONTROLLED(x-1, y-2);
  if (!IS_OUT_OF_BOARD(x-1, y+2)) MARK_CONTROLLED(x-1, y+2);
  if (!IS_OUT_OF_BOARD(x+1, y-2)) MARK_CONTROLLED(x+1, y-2);
  if (!IS_OUT_OF_BOARD(x+1, y+2)) MARK_CONTROLLED(x+1, y+2);
}

int Board::generate_rook_moves(int x, int y, int *moves)
{
  int *start = moves;
  int myColor = PLAYER_COLOR(At(x, y));
  for(int Y = y - 1; Y >= 0; --Y) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x, Y)) APPEND_MOVE(x, Y); if (At(x, Y)) break; }
  for(int Y = y + 1; Y < 8; ++Y) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x, Y)) APPEND_MOVE(x, Y); if (At(x, Y)) break; }
  for(int X = x - 1; X >= 0; --X) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, X, y)) APPEND_MOVE(X, y); if (At(X, y)) break; }
  for(int X = x + 1; X < 8; ++X) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, X, y)) APPEND_MOVE(X, y); if (At(X, y)) break; }
  return moves - start;
}

void Board::mark_rook_controlled_squares(int x, int y, int squares[8][8])
{
  for(int Y = y - 1; Y >= 0; --Y) { MARK_CONTROLLED(x, Y); if (At(x, Y)) break; }
  for(int Y = y + 1; Y < 8; ++Y) { MARK_CONTROLLED(x, Y); if (At(x, Y)) break; }
  for(int X = x - 1; X >= 0; --X) { MARK_CONTROLLED(X, y); if (At(X, y)) break; }
  for(int X = x + 1; X < 8; ++X) { MARK_CONTROLLED(X, y); if (At(X, y)) break; }
}

int Board::generate_bishop_moves(int x, int y, int *moves)
{
  int *start = moves;
  int myColor = PLAYER_COLOR(At(x, y));
  for(int i = 1; i <= MIN(  x,   y); ++i) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-i, y-i)) APPEND_MOVE(x-i, y-i); if (At(x-i, y-i)) break; }
  for(int i = 1; i <= MIN(  x, 7-y); ++i) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-i, y+i)) APPEND_MOVE(x-i, y+i); if (At(x-i, y+i)) break; }
  for(int i = 1; i <= MIN(7-x,   y); ++i) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+i, y-i)) APPEND_MOVE(x+i, y-i); if (At(x+i, y-i)) break; }
  for(int i = 1; i <= MIN(7-x, 7-y); ++i) { if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+i, y+i)) APPEND_MOVE(x+i, y+i); if (At(x+i, y+i)) break; }
  return moves - start;
}

void Board::mark_bishop_controlled_squares(int x, int y, int squares[8][8])
{
  for(int i = 1; i <= MIN(  x,   y); ++i) { MARK_CONTROLLED(x-i, y-i); if (At(x-i, y-i)) break; }
  for(int i = 1; i <= MIN(  x, 7-y); ++i) { MARK_CONTROLLED(x-i, y+i); if (At(x-i, y+i)) break; }
  for(int i = 1; i <= MIN(7-x,   y); ++i) { MARK_CONTROLLED(x+i, y-i); if (At(x+i, y-i)) break; }
  for(int i = 1; i <= MIN(7-x, 7-y); ++i) { MARK_CONTROLLED(x+i, y+i); if (At(x+i, y+i)) break; }
}

int Board::generate_queen_moves(int x, int y, int *moves)
{
  int *start = moves;
  moves += generate_rook_moves(x, y, moves);
  moves += generate_bishop_moves(x, y, moves);
  return moves - start;
}

void Board::mark_queen_controlled_squares(int x, int y, int squares[8][8])
{
  mark_rook_controlled_squares(x, y, squares);
  mark_bishop_controlled_squares(x, y, squares);
}

int Board::generate_king_moves(int x, int y, int *moves)
{
  int *start = moves;
  int myColor = PLAYER_COLOR(At(x, y));
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-1, y-1)) APPEND_MOVE(x-1, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-1,   y)) APPEND_MOVE(x-1,   y);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x-1, y+1)) APPEND_MOVE(x-1, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor,   x, y-1)) APPEND_MOVE(  x, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor,   x, y+1)) APPEND_MOVE(  x, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+1, y-1)) APPEND_MOVE(x+1, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+1,   y)) APPEND_MOVE(x+1,   y);
  if (IS_EMPTY_OR_OPPONENT_AT(myColor, x+1, y+1)) APPEND_MOVE(x+1, y+1);

  // Check castling moves.
  int castlingSide = (myColor == WHITE) ? 0 : 1;
  if ((castlingPiecesAtHome[castlingSide] & KINGSIDE_CASTLING_MASK) == KINGSIDE_CASTLING_MASK
    || (castlingPiecesAtHome[castlingSide] & QUEENSIDE_CASTLING_MASK) == QUEENSIDE_CASTLING_MASK)
  {
    int opponentControlledSquares[8][8];
    mark_controlled_squares(OPPONENT_COLOR(myColor), opponentControlledSquares);
    if (!opponentControlledSquares[y][x])
    {
      if ((castlingPiecesAtHome[castlingSide] & KINGSIDE_CASTLING_MASK) == KINGSIDE_CASTLING_MASK && !At(5, y) && !At(6, y) && !opponentControlledSquares[y][5] && !opponentControlledSquares[y][6])
        APPEND_MOVE(6, y);

      if ((castlingPiecesAtHome[castlingSide] & QUEENSIDE_CASTLING_MASK) == QUEENSIDE_CASTLING_MASK && !At(1, y) && !At(2, y) && !At(3, y) && !opponentControlledSquares[y][2] && !opponentControlledSquares[y][3])
        APPEND_MOVE(2, y);
    }
  }

  return moves - start;
}

void Board::mark_king_controlled_squares(int x, int y, int squares[8][8])
{
  if (!IS_OUT_OF_BOARD(x-1, y-1)) MARK_CONTROLLED(x-1, y-1);
  if (!IS_OUT_OF_BOARD(x-1,   y)) MARK_CONTROLLED(x-1,   y);
  if (!IS_OUT_OF_BOARD(x-1, y+1)) MARK_CONTROLLED(x-1, y+1);
  if (!IS_OUT_OF_BOARD(  x, y-1)) MARK_CONTROLLED(  x, y-1);
  if (!IS_OUT_OF_BOARD(  x, y+1)) MARK_CONTROLLED(  x, y+1);
  if (!IS_OUT_OF_BOARD(x+1, y-1)) MARK_CONTROLLED(x+1, y-1);
  if (!IS_OUT_OF_BOARD(x+1,   y)) MARK_CONTROLLED(x+1,   y);
  if (!IS_OUT_OF_BOARD(x+1, y+1)) MARK_CONTROLLED(x+1, y+1);
}

void Board::mark_controlled_squares(int color, int squares[8][8])
{
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      squares[y][x] = 0;

  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      if (PLAYER_COLOR(At(x, y)) == color)
        switch(PIECE_TYPE(At(x, y)))
        {
        case KING: mark_king_controlled_squares(x, y, squares); break;
        case QUEEN: mark_queen_controlled_squares(x, y, squares); break;
        case ROOK: mark_rook_controlled_squares(x, y, squares); break;
        case BISHOP: mark_bishop_controlled_squares(x, y, squares); break;
        case KNIGHT: mark_knight_controlled_squares(x, y, squares); break;
        case PAWN: mark_pawn_controlled_squares(x, y, squares); break;
        default: break;
        }
}

int Board::generate_moves_without_king_safety(int x, int y, int *moves)
{
  switch(PIECE_TYPE(At(x, y)))
  {
    case KING:   return generate_king_moves(x, y, moves);
    case QUEEN:  return generate_queen_moves(x, y, moves);
    case ROOK:   return generate_rook_moves(x, y, moves);
    case BISHOP: return generate_bishop_moves(x, y, moves);
    case KNIGHT: return generate_knight_moves(x, y, moves);
    case PAWN:   return generate_pawn_moves(x, y, moves);
    default:
      return 0;
  }
}

int Board::generate_moves(int x, int y, int *moves)
{
  int myColor = PLAYER_COLOR(At(x, y));
  int num = generate_moves_without_king_safety(x, y, moves);

  // Test king safety for each move
  for(int *m = moves, *e = moves + num; m != e; m += 2)
  {
    Board copy = *this;
    int dx = m[0];
    int dy = m[1];
    copy.make_move(x, y, dx, dy);
    if (copy.is_king_in_check(myColor))
    {
      m[0] = e[-2];
      m[1] = e[-1];
      e -= 2;
      num -=2;
      m -= 2;
    }
  }
  return num;
}

bool Board::find_first_piece(int piece, int *X, int *Y)
{
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
    {
      if (At(x, y) == piece)
      {
        *X = x;
        *Y = y;
        return true;
      }
    }
  return false;
}
