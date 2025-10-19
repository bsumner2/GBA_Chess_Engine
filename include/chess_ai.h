/** (C) 10 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_AI_
#define _CHESS_AI_

#include <GBAdev_types.h>
#include "chess_board.h"
#include "chess_transposition_table.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */
#define DEFAULT_MAX_DEPTH 5

#ifndef MAX_DEPTH
#define MAX_DEPTH DEFAULT_MAX_DEPTH
#endif

typedef struct s_board_state BoardState_t;

typedef struct s_move_score ChessAI_MoveSearch_Result_t;


typedef struct s_chess_ai_params {
  TranspositionTable_t *ttable;
  BoardState_t *root_state;
  Move_Validation_Flag_e last_move;
  u16 depth;
  u8 gen, team;
} ChessAI_Params_t;





IWRAM_CODE void ChessAI_Params_Init(ChessAI_Params_t *obj, BoardState_t 
    *root_state, 
    int depth);

IWRAM_CODE void ChessAI_Move(ChessAI_Params_t *ai_params,
                             ChessAI_MoveSearch_Result_t *return_move);

EWRAM_CODE BoardState_t *BoardState_Alloc(void);
EWRAM_CODE void BoardState_Dealloc(BoardState_t *board_state);
EWRAM_CODE BoardState_t *BoardState_FromCtx(BoardState_t *board_state, 
                                            const ChessGameCtx_t *ctx);


#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_AI_ */
