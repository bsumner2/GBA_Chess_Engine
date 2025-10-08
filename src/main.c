/** (C) Burt Sumner 2025 */
#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>
#include "key_status.h"
#include "mode3_io.h"
#include "subpixel.h"
#include "chess_board.h"

#define ALL_KEYS KEY_STAT_KEYS_MASK

extern IWRAM_CODE void ChessGameloop_ISR_Handler(void);
extern void IRQ_Sync(u32 flags);
extern u32 ChessGameLoop(ChessGameCtx_t*);

static ChessGameCtx_t context = {0};

#ifdef TEST_KNIGHT_MVMT
extern void ChessGame_AnimateMove(ChessGameCtx_t *ctx,
                                  ChessPiece_Data_t *moving,
                                  ChessPiece_Data_t *captured);
#endif

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
  REG_IE |= (1<<IRQ_VBLANK)|(1<<IRQ_KEYPAD);
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
  REG_IE|=(1<<IRQ_VBLANK);
  REG_IME=1;
  mode3_printf(0,0,0x10A5, "sizeof(\x1b[0x328A]ChessBoard_Row_e\x1b[0x10A5]) = \x1b[0x2E1F]%zu\x1b[0x10A5]", sizeof(ChessBoard_Row_e));
  do SUPERVISOR_CALL(0x05); while (1);

#else
  u32 outcome;
  do {
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
    REG_IE |= (1<<IRQ_VBLANK)|(1<<IRQ_KEYPAD);
    REG_IME = 1;
    outcome = ChessGameLoop(&context);
    Fast_Memset32(VRAM_M3, 0, sizeof(u16)*M3_SCREEN_HEIGHT*M3_SCREEN_WIDTH/sizeof(WORD));
    REG_DPY_CNT = REG_VALUE(DPY_CNT, MODE, 3)|REG_FLAG(DPY_CNT, BG2);
    if (WHITE_FLAGBIT&outcome) {
      mode3_printf((M3_SCREEN_WIDTH-11*SubPixel_Glyph_Width)/2,(M3_SCREEN_HEIGHT-SubPixel_Glyph_Height)/2,0x10A5, "White Wins!");
    } else if (BLACK_FLAGBIT&outcome) {
      mode3_printf((M3_SCREEN_WIDTH-11*SubPixel_Glyph_Width)/2,(M3_SCREEN_HEIGHT-SubPixel_Glyph_Height)/2,0x10A5, "Black Wins!");
    } else {
      mode3_printf((M3_SCREEN_WIDTH-10*SubPixel_Glyph_Width)/2,(M3_SCREEN_HEIGHT-SubPixel_Glyph_Height)/2,0x10A5, "Stalemate!");
    }
    do IRQ_Sync(1<<IRQ_KEYPAD); while (!KEY_STROKE(START));
    ChessGameCtx_Close(&context);
  } while (1);
#endif
}
