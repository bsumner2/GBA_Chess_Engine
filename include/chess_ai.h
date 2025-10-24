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
#define DEFAULT_MAX_DEPTH 2

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

#define CONVERT_CHESS_AI_TEAM_FLAG(team) ((team)<<8)

IWRAM_CODE void ChessAI_Params_Init(ChessAI_Params_t *obj,
                                    BoardState_t *root_state, 
                                    int depth,
                                    u32 team);
#define ChessAI_Params_Uninit(obj)\
  Fast_Memset32(obj, 0, sizeof(ChessAI_Params_t)/sizeof(WORD))

IWRAM_CODE void ChessAI_Move(ChessAI_Params_t *ai_params,
                             ChessAI_MoveSearch_Result_t *return_move);




#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_AI_ */
