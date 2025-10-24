/** (C) 11 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_MOVE_ITERATOR_
#define _CHESS_MOVE_ITERATOR_

#include "chess_board.h"
#include "chess_ai_types.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */


typedef struct s_chess_move_iterator_private ChessMoveIterator_PrivateFields_t;
typedef struct s_chess_move_iterator {
  u32 size;
  ChessMoveIterator_PrivateFields_t *priv;
} ChessMoveIterator_t;

#define MV_ITER_MOVESET_SET_TYPE(mode)\
  (mode&MV_ITER_MOVESET_SET_TYPE_MASK)
#define MV_ITER_MOVESET_ORDERED(mode)\
  (0!=(mode&MV_ITER_MOVESET_ORDERED_FLAGBIT))
#define MV_ITER_MOVESET_UNORDERED(mode)\
  (0==(mode&MV_ITER_MOVESET_ORDERED_FLAGBIT))

typedef enum e_chess_move_iterator_moveset_mode {
  MV_ITER_MOVESET_ALL_MINUS_ALLIED_COLLISIONS=0,
  MV_ITER_MOVESET_COLLISIONS_ONLY_SET=1,
  MV_ITER_MOVESET_ALL_SET=2,
  MV_ITER_MOVESET_SET_TYPE_MASK=3,
  MV_ITER_MOVESET_ORDERED_FLAGBIT=4,
} ChessMoveIterator_MoveSetMode_e;



EWRAM_CODE BOOL ChessMoveIterator_Alloc(ChessMoveIterator_t *dst_iterator,
                             ChessBoard_Idx_t piece_location,
                             const BoardState_t *state,
                             ChessMoveIterator_MoveSetMode_e ordering);

EWRAM_CODE BOOL ChessMoveIterator_HasNext(const ChessMoveIterator_t *iterator);
EWRAM_CODE BOOL ChessMoveIterator_Next(ChessMoveIterator_t *iterator,
                            ChessMoveIteration_t *ret_mv);
EWRAM_CODE BOOL ChessMoveIterator_Dealloc(ChessMoveIterator_t *iterator);

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_MOVE_ITERATOR_ */
