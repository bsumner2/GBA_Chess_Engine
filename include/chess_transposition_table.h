/** (C) 9 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_TRANSPOSITION_TABLE_
#define _CHESS_TRANSPOSITION_TABLE_
#include "chess_board.h"
#include "chess_board_state.h"
#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#define TTABLE_CLUSTER_SIZE 2
#define TTABLE_SIZE (1<<12)
#define TTABLE_IDX_MASK (TTABLE_SIZE-1)
typedef struct s_move_score {
    i16 score;
    ChessBoard_Idx_Compact_t start,dst;
    Move_Validation_Flag_e mv_flags;
    ChessPiece_e promo;
} TTable_BestMove_Score_t;

typedef struct s_transposition_table_ent {
  u64 key;
  TTable_BestMove_Score_t best_move;
  u8 depth, gen;
} ALIGN(8) TTableEnt_t, TranspositionTable_Entry_t;


typedef struct s_transposition_table_slot {
  TranspositionTable_Entry_t buckets[TTABLE_CLUSTER_SIZE];
} TTableSlot_t, TranspositionTable_Slot_t;

typedef struct s_transposition_table {
  TTableSlot_t slots[TTABLE_SIZE];
  u8 generation;
} TTable_t, TranspositionTable_t;



IWRAM_CODE BOOL TTable_Probe(const TTable_t *tt, 
                             TTableEnt_t *query_entry) ;

IWRAM_CODE void TTable_Insert(TTable_t *tt, const TTableEnt_t *entry);
#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_TRANSPOSITION_TABLE_ */
