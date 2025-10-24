/** (C) 13 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_BOARD_STATE_
#define _CHESS_BOARD_STATE_

#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#include "chess_board.h"
#include "chess_ai_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

EWRAM_CODE BoardState_t *BoardState_Alloc(void);
EWRAM_CODE void BoardState_Dealloc(BoardState_t *board_state);
EWRAM_CODE BoardState_t *BoardState_FromCtx(BoardState_t *board_state,
                                            const ChessGameCtx_t *ctx);
EWRAM_CODE BoardState_t *BoardState_ApplyMove(BoardState_t *board_state,
                                       const ChessMoveIteration_t *move_data,
                                       ChessBoard_Idx_Compact_t start_pos);

EWRAM_CODE BoardState_t *BoardState_Alloc(void);
EWRAM_CODE void BoardState_Dealloc(BoardState_t *board_state);
EWRAM_CODE BoardState_t *BoardState_FromCtx(BoardState_t *board_state, 
                                            const ChessGameCtx_t *ctx);

#define BoardState_GetPiece_CompactIdx(board_state, compact_coord)\
  BoardState_GetPiece(board_state,\
                      BOARD_IDX_CONVERT(compact_coord, NORMAL_IDX_TYPE))
ChessPiece_e BoardState_GetPiece(const BoardState_t *board_state,
                                 const ChessBoard_Idx_t coord); 

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_BOARD_STATE_ */
