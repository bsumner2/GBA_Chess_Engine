/** (C) Burt Sumner 2025 */
#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>

#include "chess_board.h"

#define ALL_KEYS KEY_STAT_KEYS_MASK

extern IWRAM_CODE void ChessGameloop_ISR_Handler(void);
extern u32 ChessGameLoop(ChessGameCtx_t*);

static ChessGameCtx_t context = {0};

int main(void) {
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
  ChessGameLoop(&context);
//  do SUPERVISOR_CALL(0x05); while (1);
}
