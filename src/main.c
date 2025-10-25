/** (C) Burt Sumner 2025 */
#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>
#include "key_status.h"
#include "mode3_io.h"
#include "subpixel.h"
#include "chess_board.h"
#include "chess_gameloop.h"

#define ALL_KEYS KEY_STAT_KEYS_MASK

#define SUBPIXEL_FONT_TEXT_HPOS_CENTERED(textlen)\
  ((M3_SCREEN_WIDTH-(textlen)*SubPixel_Glyph_Width)/2)

#define SUBPIXEL_FONT_TEXT_VPOS_CENTERED\
  ((M3_SCREEN_HEIGHT-SubPixel_Glyph_Height)/2)

static ChessGameCtx_t context = {0};
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
#define LSTRLEN(literal) (sizeof(literal)-1)
// This macro + assertion of macro is basically my lazy way of writing front 
// end code to tell user that this feature is yet to be implemented
#define CPU_X_CPU_IMPLEMENTED FALSE
TODO("Implement CPU X CPU mode")
#define SEL_COUNT 4
#define SEL_MASK 3
typedef enum e_game_mode_sel {
  GAME_MODE_2PLAYER=0,
  GAME_MODE_1PLAYER_V_CPU,
  GAME_MODE_CPU_V_1PLAYER,
  GAME_MODE_CPU_V_CPU
} GameModeSelection_e;
static const char *MODE_SEL_PROMPTS[4] = {
  "2-player mode",
  "1-player X CPU",
  "CPU X 1-player",
  "CPU X CPU",
};

static const size_t MODE_SEL_PROMPT_LENS[4] = {
  LSTRLEN("2-player mode"),
  LSTRLEN("1-player X CPU"),
  LSTRLEN("CPU X 1-player"),
  LSTRLEN("CPU X CPU")
};

#define SELECT_CLR 0x4167
#define NORMAL_CLR 0x1484

static int M3FE_SelGamemode(void);
static void M3_SelScreen(int sel, int prev);
void M3_SelScreen(int sel, int prev) {
  assert((SEL_MASK&sel)==sel);
  if (0 > prev) {
    for (int i = 0; SEL_COUNT > i; ++i) {
      if ((const int)sel==i) {
        mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[i]),
                     ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*i,
                     SELECT_CLR,
                     "%s", // Pass it as a format arg to get rid of warning
                     MODE_SEL_PROMPTS[i]);
      } else {
        mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[i]),
                     ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*i,
                     NORMAL_CLR,
                     "%s",  // Pass it as a format arg to get rid of warning
                     MODE_SEL_PROMPTS[i]);
      }
    }
    return;
  }
  assert((SEL_MASK&prev)==prev);
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[sel]),
               ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*sel,
               SELECT_CLR,
               "%s",  // Pass it as a format arg to get rid of warning
               MODE_SEL_PROMPTS[sel]);

  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[prev]),
               ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*prev,
               NORMAL_CLR,
               "%s",  // Pass it as a format arg to get rid of warning
               MODE_SEL_PROMPTS[prev]);
}

int M3FE_SelGamemode(void) {
  int prev=0, cur=0;
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);
  M3_SelScreen(0, -1);
  for (IRQ_Sync(IRQ_FLAG(KEYPAD)); !KEY_STROKE(A); IRQ_Sync(IRQ_FLAG(KEYPAD))) {
    if (KEY_STROKE(UP)) {
      --cur;
    } else if (KEY_STROKE(DOWN)) {
      ++cur;
    } else {
      continue;
    }
    cur&=SEL_MASK;
    M3_SelScreen(cur, prev);
    prev = cur;
  }

  Fast_Memset32(VRAM_M3,
                0,
                sizeof(u16)*M3_SCREEN_HEIGHT*M3_SCREEN_WIDTH/sizeof(WORD));

  switch ((GameModeSelection_e)cur) {
    case GAME_MODE_2PLAYER:
      return 0;
      break;
    case GAME_MODE_1PLAYER_V_CPU:
    case GAME_MODE_CPU_V_1PLAYER:
      return PIECE_TEAM_MASK^(cur<<12);
      break;
    case GAME_MODE_CPU_V_CPU:
      assert(CPU_X_CPU_IMPLEMENTED);
      return PIECE_TEAM_MASK;
      break;
    default:
      assert(SEL_MASK&cur);
  }

}

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
    if (0!=cpu_team_side) {
      assert(PIECE_TEAM_MASK!=cpu_team_side);
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
    Fast_Memset32(VRAM_M3, 0, sizeof(u16)*M3_SCREEN_HEIGHT*M3_SCREEN_WIDTH/sizeof(WORD));
    REG_DPY_CNT = REG_VALUE(DPY_CNT, MODE, 3)|REG_FLAG(DPY_CNT, BG2);
    if (WHITE_FLAGBIT&outcome) {
      mode3_printf(
          SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(WHITE_WIN_MSG)),
          SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
          0x10A5, 
          WHITE_WIN_MSG);
    } else if (BLACK_FLAGBIT&outcome) {
      mode3_printf(
          SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(BLACK_WIN_MSG)),
          SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
          0x10A5,
          BLACK_WIN_MSG);
    } else {
      mode3_printf(
          SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN(STALEMATE_MSG)),
          SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
          0x10A5,
          STALEMATE_MSG);
    }
    do IRQ_Sync(IRQ_FLAG(KEYPAD)); while (!KEY_STROKE(START));
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
