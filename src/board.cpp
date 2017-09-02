#include "board.h"

// Returns true if there is a white piece at the given coordinates
#define IS_WHITE_AT(x,y) (IS_WHITE_PIECE(At((x),(y))))

// Returns true if there is a black piece at the given coordinates
#define IS_BLACK_AT(x,y) (IS_BLACK_PIECE(At((x),(y))))

// Returns true if there is an empty square or a piece of opponent's color at the given coordinates
#define IS_EMPTY_OR_OPPONENT_AT(playerColor, x, y) IS_EMPTY_OR_OPPONENT(playerColor, At((x), (y)))

// Returns true if there is a piece of opponent's color at the given coordinates
#define IS_OPPONENT_AT(playerColor, x, y) IS_OPPONENT(playerColor, At((x), (y)))

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define ABS(x) ((x) >= 0 ? (x) : -(x))

void Board::new_game()
{
  out = OUT_OF_BOARD;
  currentPlayer = WHITE;
  enpassantX = enpassantY = -1;
  castlingPiecesAtHome[0] = castlingPiecesAtHome[1] = KING_AT_HOME | KING_ROOK_AT_HOME | QUEEN_ROOK_AT_HOME;

  board[0][0] = WHITE_ROOK;
  board[0][1] = WHITE_KNIGHT;
  board[0][2] = WHITE_BISHOP;
  board[0][3] = WHITE_QUEEN;
  board[0][4] = WHITE_KING;
  board[0][5] = WHITE_BISHOP;
  board[0][6] = WHITE_KNIGHT;
  board[0][7] = WHITE_ROOK;

  board[7][0] = BLACK_ROOK;
  board[7][1] = BLACK_KNIGHT;
  board[7][2] = BLACK_BISHOP;
  board[7][3] = BLACK_QUEEN;
  board[7][4] = BLACK_KING;
  board[7][5] = BLACK_BISHOP;
  board[7][6] = BLACK_KNIGHT;
  board[7][7] = BLACK_ROOK;

  for(int x = 0; x < 8; ++x)
  {
    board[1][x] = WHITE_PAWN;
    board[6][x] = BLACK_PAWN;
    for(int y = 2; y < 6; ++y)
      board[y][x] = 0;
  }
}

#define MOVE(sx, sy, dx, dy) do { board[dy][dx] = board[sy][sx]; board[sy][sx] = 0; } while(0)

void Board::make_move(int srcX, int srcY, int dstX, int dstY)
{
  int pieceColor = PLAYER_COLOR(board[srcY][srcX]);
  int pieceType = PIECE_TYPE(board[srcY][srcX]);
  int castlingSide = (pieceColor == WHITE) ? 0 : 1;
  int castlingRank = (pieceColor == WHITE) ? 0 : 7;
  MOVE(srcX, srcY, dstX, dstY);

  // Did we make an en passant capture?
  if (pieceType == PAWN && dstX == enpassantX && dstY == enpassantY) board[srcY][dstX] = 0; // If so, remove the pawn that moved two squares forward
  enpassantX = enpassantY = -1; // Clear en passant state, the opportunity do en passant captures is over.

  switch(pieceType)
  {
  case PAWN: // If we move a pawn forward two squares, record an en passant square that will perform a capture of this pawn (next move only)
    if (ABS(dstY-srcY) == 2) enpassantX = srcX, enpassantY = (srcY + dstY) / 2;
    // Auto-promote pawns to queen. (TODO: Add UI dialog to ask what to promote to)
    if (dstY == 0 || dstY == 7) board[dstY][dstX] = pieceColor | QUEEN;
    break;
  case KING:
    if (ABS(dstX-srcX) > 1) MOVE((dstX < srcX) ? 0 : 7, dstY, (srcX+dstX)/2, dstY); // If king is castling, handle moving the rook here
    castlingPiecesAtHome[castlingSide] &= ~KING_AT_HOME;
    break;
  case ROOK:
    // One of the rooks moved that could have castled?
    if (srcX == 0 && srcY == castlingRank) castlingPiecesAtHome[castlingSide] &= ~QUEEN_ROOK_AT_HOME;
    else if (srcX == 7 && srcY == castlingRank) castlingPiecesAtHome[castlingSide] &= ~KING_ROOK_AT_HOME;
    break;
  }
  currentPlayer = OPPONENT_COLOR(pieceColor);
}

bool Board::is_king_in_check(int color)
{
  int opponentControlledSquares[8][8];
  mark_controlled_squares(OPPONENT_COLOR(color), opponentControlledSquares);

  int kingX, kingY;
  find_king(color == WHITE ? WHITE_KING : BLACK_KING, &kingX, &kingY);
  return opponentControlledSquares[kingY][kingX] != 0;
}

#define APPEND_MOVE(x, y) do { *moves++ = x; *moves++ = y; } while(0)
#define MARK_CONTROLLED(x, y) squares[y][x] = 1

int *Board::generate_pawn_moves(int pieceColor, int x, int y, int *moves)
{
  int dir = (pieceColor == WHITE) ? 1 : -1;
  if (!At(x, y+dir))
  {
    const int homeRow = (pieceColor == WHITE) ? 1 : 6;
    APPEND_MOVE(x, y+dir);
    if (y == homeRow && !At(x, y+dir+dir)) APPEND_MOVE(x, y+dir+dir);
  }
  if (IS_OPPONENT_AT(pieceColor, x-1, y+dir) || (x-1 == enpassantX && y+dir == enpassantY)) APPEND_MOVE(x-1, y+dir);
  if (IS_OPPONENT_AT(pieceColor, x+1, y+dir) || (x+1 == enpassantX && y+dir == enpassantY)) APPEND_MOVE(x+1, y+dir);
  return moves;
}

void Board::mark_pawn_controlled_squares(int x, int y, int squares[8][8])
{
  int dir = (PLAYER_COLOR(board[y][x]) == WHITE) ? 1 : -1;
  if (!IS_OUT_OF_BOARD(x-1, y+dir)) MARK_CONTROLLED(x-1, y+dir);
  if (!IS_OUT_OF_BOARD(x+1, y+dir)) MARK_CONTROLLED(x+1, y+dir);
}

int *Board::generate_knight_moves(int pieceColor, int x, int y, int *moves)
{
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-2, y-1)) APPEND_MOVE(x-2, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-2, y+1)) APPEND_MOVE(x-2, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+2, y-1)) APPEND_MOVE(x+2, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+2, y+1)) APPEND_MOVE(x+2, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-1, y-2)) APPEND_MOVE(x-1, y-2);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-1, y+2)) APPEND_MOVE(x-1, y+2);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+1, y-2)) APPEND_MOVE(x+1, y-2);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+1, y+2)) APPEND_MOVE(x+1, y+2);
  return moves;
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

int *Board::generate_rook_moves(int pieceColor, int x, int y, int *moves)
{
  for(int Y = y - 1; Y >= 0; --Y) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[Y][x])) APPEND_MOVE(x, Y); if (board[Y][x]) break; }
  for(int Y = y + 1; Y < 8; ++Y) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[Y][x])) APPEND_MOVE(x, Y); if (board[Y][x]) break; }
  for(int X = x - 1; X >= 0; --X) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y][X])) APPEND_MOVE(X, y); if (board[y][X]) break; }
  for(int X = x + 1; X < 8; ++X) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y][X])) APPEND_MOVE(X, y); if (board[y][X]) break; }
  return moves;
}

void Board::mark_rook_controlled_squares(int x, int y, int squares[8][8])
{
  for(int Y = y - 1; Y >= 0; --Y) { MARK_CONTROLLED(x, Y); if (board[Y][x]) break; }
  for(int Y = y + 1; Y < 8; ++Y) { MARK_CONTROLLED(x, Y); if (board[Y][x]) break; }
  for(int X = x - 1; X >= 0; --X) { MARK_CONTROLLED(X, y); if (board[y][X]) break; }
  for(int X = x + 1; X < 8; ++X) { MARK_CONTROLLED(X, y); if (board[y][X]) break; }
}

int *Board::generate_bishop_moves(int pieceColor, int x, int y, int *moves)
{
  for(int i = 1; i <= MIN(  x,   y); ++i) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y-i][x-i])) APPEND_MOVE(x-i, y-i); if (board[y-i][x-i]) break; }
  for(int i = 1; i <= MIN(  x, 7-y); ++i) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y+i][x-i])) APPEND_MOVE(x-i, y+i); if (board[y+i][x-i]) break; }
  for(int i = 1; i <= MIN(7-x,   y); ++i) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y-i][x+i])) APPEND_MOVE(x+i, y-i); if (board[y-i][x+i]) break; }
  for(int i = 1; i <= MIN(7-x, 7-y); ++i) { if (IS_EMPTY_OR_OPPONENT(pieceColor, board[y+i][x+i])) APPEND_MOVE(x+i, y+i); if (board[y+i][x+i]) break; }
  return moves;
}

void Board::mark_bishop_controlled_squares(int x, int y, int squares[8][8])
{
  for(int i = 1; i <= MIN(  x,   y); ++i) { MARK_CONTROLLED(x-i, y-i); if (board[y-i][x-i]) break; }
  for(int i = 1; i <= MIN(  x, 7-y); ++i) { MARK_CONTROLLED(x-i, y+i); if (board[y+i][x-i]) break; }
  for(int i = 1; i <= MIN(7-x,   y); ++i) { MARK_CONTROLLED(x+i, y-i); if (board[y-i][x+i]) break; }
  for(int i = 1; i <= MIN(7-x, 7-y); ++i) { MARK_CONTROLLED(x+i, y+i); if (board[y+i][x+i]) break; }
}

int *Board::generate_queen_moves(int pieceColor, int x, int y, int *moves)
{
  moves = generate_rook_moves(pieceColor, x, y, moves);
  return generate_bishop_moves(pieceColor, x, y, moves);
}

void Board::mark_queen_controlled_squares(int x, int y, int squares[8][8])
{
  mark_rook_controlled_squares(x, y, squares);
  mark_bishop_controlled_squares(x, y, squares);
}

int *Board::generate_king_moves(int pieceColor, int x, int y, int *moves)
{
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-1, y-1)) APPEND_MOVE(x-1, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-1,   y)) APPEND_MOVE(x-1,   y);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x-1, y+1)) APPEND_MOVE(x-1, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor,   x, y-1)) APPEND_MOVE(  x, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor,   x, y+1)) APPEND_MOVE(  x, y+1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+1, y-1)) APPEND_MOVE(x+1, y-1);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+1,   y)) APPEND_MOVE(x+1,   y);
  if (IS_EMPTY_OR_OPPONENT_AT(pieceColor, x+1, y+1)) APPEND_MOVE(x+1, y+1);

  // Check castling moves.
  int castlingSide = (pieceColor == WHITE) ? 0 : 1;
  if ((castlingPiecesAtHome[castlingSide] & KINGSIDE_CASTLING_MASK) == KINGSIDE_CASTLING_MASK || (castlingPiecesAtHome[castlingSide] & QUEENSIDE_CASTLING_MASK) == QUEENSIDE_CASTLING_MASK)
  {
    int opponentControlledSquares[8][8];
    mark_controlled_squares(OPPONENT_COLOR(pieceColor), opponentControlledSquares);
    if (!opponentControlledSquares[y][x])
    {
      if ((castlingPiecesAtHome[castlingSide] & KINGSIDE_CASTLING_MASK) == KINGSIDE_CASTLING_MASK && !board[y][5] && !board[y][6] && !opponentControlledSquares[y][5] && !opponentControlledSquares[y][6])
        APPEND_MOVE(6, y);
      if ((castlingPiecesAtHome[castlingSide] & QUEENSIDE_CASTLING_MASK) == QUEENSIDE_CASTLING_MASK && !board[y][1] && !board[y][2] && !board[y][3] && !opponentControlledSquares[y][2] && !opponentControlledSquares[y][3])
        APPEND_MOVE(2, y);
    }
  }
  return moves;
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
      if (PLAYER_COLOR(board[y][x]) == color)
        switch(PIECE_TYPE(board[y][x]))
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

int *Board::generate_moves_without_king_safety(int pieceColor, int x, int y, int *moves)
{
  switch(PIECE_TYPE(board[y][x]))
  {
  case KING:   return generate_king_moves(pieceColor, x, y, moves);
  case QUEEN:  return generate_queen_moves(pieceColor, x, y, moves);
  case ROOK:   return generate_rook_moves(pieceColor, x, y, moves);
  case BISHOP: return generate_bishop_moves(pieceColor, x, y, moves);
  case KNIGHT: return generate_knight_moves(pieceColor, x, y, moves);
  case PAWN:   return generate_pawn_moves(pieceColor, x, y, moves);
  default:     return moves;
  }
}

int *Board::generate_moves(int x, int y, int *moves)
{
  int pieceColor = PLAYER_COLOR(board[y][x]);
  int *end = generate_moves_without_king_safety(pieceColor, x, y, moves);
  while(moves != end) // Test king safety for each move
  {
    Board copy = *this;
    copy.make_move(x, y, moves[0], moves[1]);
    if (copy.is_king_in_check(pieceColor))
    {
      moves[1] = *--end;
      moves[0] = *--end;
    }
    else moves += 2;
  }
  return end;
}

bool Board::is_valid_move(int srcX, int srcY, int dstX, int dstY)
{
  int moves[48*2];
  int *end = generate_moves(srcX, srcY, moves);
  for(int *m = moves; m != end; m += 2)
    if (m[0] == dstX && m[1] == dstY)
      return true;
  return false;
}

bool Board::has_valid_moves(int x, int y)
{
  int moves[48*2];
  int *end = generate_moves(x, y, moves);
  return end != moves;
}

void Board::find_king(int kingPiece, int *X, int *Y)
{
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      if (board[y][x] == kingPiece)
      {
        *X = x, *Y = y;
        return;
      }
}
