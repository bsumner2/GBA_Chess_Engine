#include "chess_ai.h"
#include "chess_board.h"

struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};

ChessPiece_e BoardState_GetPiece(const BoardState_t *board_state,
                                            const ChessBoard_Idx_t coord) {
  return board_state->board[BOARD_IDX(coord)];
}
