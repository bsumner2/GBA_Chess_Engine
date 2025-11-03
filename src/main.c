/** (C) Burt Sumner 2025 */
#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>
#include <sys/reent.h>
#include "key_status.h"
#include "mode3_io.h"
#include "chess_game_frontend.h"
#include "chess_board.h"
#include "chess_gameloop.h"

#define ALL_KEYS KEY_STAT_KEYS_MASK



ChessGameCtx_t context = {0};
static ChessAI_Params_t ai = {0};
static BoardState_t *ai_board_state_tracker=NULL;
static u32 outcome = 0;
static ChessPiece_e cpu_team_side = 0;
#ifdef TEST_KNIGHT_MVMT
extern void ChessGame_AnimateMove(ChessGameCtx_t *ctx,
                                  ChessPiece_Data_t *moving,
                                  ChessPiece_Data_t *captured);
#endif
#define STALEMATE_MSG "Stalemate!"
#define WHITE_WIN_MSG "White Wins!"
#define BLACK_WIN_MSG "Black Wins!"


/** LSTRLEN has to be sizeof(literal)-1 because size of string literal includes
 * the null terminator char at end of string buffer.
 **/





int main(void) {
#ifdef TEST_KNIGHT_MVMT
  ChessBG_Init();
  ChessGameCtx_Init(&context);
  REG_BLEND_CNT = REG_FLAG(BLEND_CNT, LAYER_B_BG0)
                    | REG_VALUE(BLEND_CNT, BLEND_MODE, BLEND_MODE_ALPHA);
  REG_BLEND_ALPHA = REG_VALUE(BLEND_ALPHA, LAYER_B_WEIGHT, 4)|REG_VALUE(BLEND_ALPHA, LAYER_A_WEIGHT, 10);
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG0) 
                    | REG_FLAG(DPY_CNT, OBJ)
                    | REG_FLAG(DPY_CNT, OBJ_1D);

  REG_IME = 0;
  REG_DPY_STAT |= REG_FLAG(DPY_STAT, VBL_IRQ);
  REG_KEY_CNT |= REG_FLAG(KEY_CNT, BWISE_AND)|REG_FLAG(KEY_CNT, IRQ)|ALL_KEYS;
  REG_KEY_CNT &= (u16)~(REG_FLAG(KEY_CNT, BWISE_AND)
        | KEY_L
        | KEY_R
        | KEY_SEL
      );
  REG_ISR_MAIN = ChessGameloop_ISR_Handler;
  REG_IE |= (IRQ_FLAG(VBLANK))|(IRQ_FLAG(KEYPAD));
  REG_IME = 1;
  context.move_selections[0].coord = (struct s_chess_coord) {
    .x=FILE_B, .y=ROW_1
  };
  context.move_selections[1].coord = (struct s_chess_coord) {
    .x = FILE_C, .y = ROW_3
  };
  ChessPiece_Data_t query = {0}, *moving_vert;
  query.location = context.move_selections[0];
  GraphNode_t *moving = Graph_Get_Vertex(context.tracker.piece_graph, &query);
  assert(NULL!=moving);
  moving_vert = moving->data;
  assert(NULL!=moving_vert);
  ChessGame_AnimateMove(&context, moving_vert, NULL);
  do SUPERVISOR_CALL(0x05); while (1);
#elif 0
#include "mode3_io.h"
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);
  REG_IME=0;
  REG_DPY_STAT|=REG_FLAG(DPY_STAT, VBL_IRQ);
  REG_ISR_MAIN = ISR_Handler_Basic;
  REG_IE|=(IRQ_FLAG(VBLANK));
  REG_IME=1;
  mode3_printf(0,0,0x10A5, "sizeof(\x1b[0x328A]ChessBoard_Row_e\x1b[0x10A5]) = \x1b[0x2E1F]%zu\x1b[0x10A5]", sizeof(ChessBoard_Row_e));
  do SUPERVISOR_CALL(0x05); while (1);

#else
  do {
    REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);
    REG_IME = 0;
    REG_DPY_STAT |= REG_FLAG(DPY_STAT, VBL_IRQ);
    REG_KEY_CNT |= REG_FLAGS(KEY_CNT, IRQ)|ALL_KEYS;

    REG_KEY_CNT &= ~REG_FLAGS(KEY_CNT, BWISE_AND, L, R, SEL);

    REG_ISR_MAIN = ChessGameloop_ISR_Handler;
    REG_IE |= IRQ_FLAGS(VBLANK, KEYPAD);
    REG_IME = 1;

    cpu_team_side = M3FE_SelGamemode();

    REG_BLEND_CNT = REG_FLAG(BLEND_CNT, LAYER_B_BG0)
                      | REG_VALUE(BLEND_CNT, BLEND_MODE, BLEND_MODE_ALPHA);
    REG_BLEND_ALPHA = REG_VALUES(BLEND_ALPHA, LAYER_B_WEIGHT, 4, LAYER_A_WEIGHT, 10);
    REG_DPY_CNT = REG_FLAGS(DPY_CNT, BG0, OBJ, OBJ_1D);
    ChessBG_Init();
    ChessGameCtx_Init(&context);
    
    atexit(ChessMoveHistory_Save);
    if (0!=cpu_team_side) {
//      assert(PIECE_TEAM_MASK!=cpu_team_side);
      ai_board_state_tracker = BoardState_Alloc();
      assert(ai_board_state_tracker!=NULL);
      ChessAI_Params_Init(&ai,
                          BoardState_FromCtx(ai_board_state_tracker, &context),
                          MAX_DEPTH,
                          cpu_team_side);
      outcome = ChessGame_AIXHuman_Loop(&context, &ai);
    } else {
      outcome = ChessGame_HumanXHuman_Loop(&context);
    }
    M3_CLR_SCREEN();
    REG_DPY_CNT = REG_VALUE(DPY_CNT, MODE, 3)|REG_FLAG(DPY_CNT, BG2);
    if (WHITE_FLAGBIT&outcome) {
      mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(WHITE_WIN_MSG)),
                   SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
                   0x10A5,
                   WHITE_WIN_MSG);
    } else if (BLACK_FLAGBIT&outcome) {
      mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(BLACK_WIN_MSG)),
                   SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
                   0x10A5,
                   BLACK_WIN_MSG);
    } else {
      mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(STALEMATE_MSG)),
                   SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
                   0x10A5,
                   STALEMATE_MSG);
    }
    do IRQ_Sync(IRQ_FLAG(KEYPAD)); while (!KEY_STROKE(START));
    ChessMoveHistory_Save();
    ChessGameCtx_Close(&context);
    if (ai_board_state_tracker) {
      ChessAI_Params_Uninit(&ai);
      BoardState_Dealloc(ai_board_state_tracker);
      ai_board_state_tracker = NULL;
      cpu_team_side = 0;
    }
  } while (1);
#endif
}
