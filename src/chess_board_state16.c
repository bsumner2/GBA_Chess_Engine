/** 
 * (C) Burt O Sumner 2025 Authorship rights reserved.
 * Free to use so long as this credit comment remains visible here.
 **/
#define __TRANSPARENT_BOARD_STATE__

#include "chess_ai.h"
#include "chess_board.h"



ChessPiece_e BoardState_GetPiece(const BoardState_t *board_state,
                                            const ChessBoard_Idx_t coord) {
  return board_state->board[BOARD_IDX(coord)];
}
