/** (C) 16 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_BOARD_STATE_ANALYSIS_
#define _CHESS_BOARD_STATE_ANALYSIS_

#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#include "chess_board.h"
#include "chess_board_state.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

/**
 * @brief Given board state, the ID of king we are detecting check for, and
 *  opposition index offset.
 * @return TRUE if king at idx allied_king_id in check. ELSE FALSE
 */
EWRAM_CODE BOOL BoardState_KingInCheck(
                                     const BoardState_t *board_state,
                                     u32 allied_king_id,
                                     u32 opp_ofs);

/**
 * @brief Check if piece is pinned to shielding king from check. If so it 
 * returns which direction the king is w.r.t to attacker and pinned piece.
 * @return The direction of pin, ELSE INVALID_MVMT_FLAGBIT.
 */
EWRAM_CODE Mvmt_Dir_e BoardState_PiecePinDirection(
                                               const BoardState_t *board_state,
                                               ChessPiece_e piece_id);

#define BoardState_PiecePinned(board_state, piece_id)\
    (INVALID_MVMT_FLAGBIT!=BoardState_PiecePinDirection(board_state, piece_id))



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_BOARD_STATE_ANALYSIS_ */
