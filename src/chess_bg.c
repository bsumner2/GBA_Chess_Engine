/** (C) Burt Sumner 2025 */
#include <GBAdev_functions.h>
#include <GBAdev_memmap.h>
#include <GBAdev_types.h>

#include "chess_board.h"
#include "chess_sprites.h"
#include "Chess_Board_BG.h"


#define SCR_ENT_IDX(x, y) (x + y*32)
#define BG_UNSEL_PALBANK 0
#define BG_SEL_PALBANK   1

void ChessBG_Init(void) {
  Fast_Memcpy32(PAL_MEM_BG, Chess_Board_BGPal, Chess_Board_BGPalLen/sizeof(WORD));
  Fast_Memcpy32(&TILE8_MEM[0][0], Chess_Board_BGTiles, Chess_Board_BGTilesLen/sizeof(WORD));
  Fast_Memcpy32(&SCR_ENT_MEM[30], Chess_Board_BGMap, Chess_Board_BGMapLen/sizeof(WORD));
  
  REG_BG0_CNT |= REG_VALUE(BG_CNT, SCR_BLOCK_BASE, 30)|REG_VALUE(BG_CNT, PRIORITY, 2);
//  REG_BG0_HOFS = 216;
  REG_BG0_HOFS = 256 - CHESS_BOARD_X_OFS;
//  REG_BG0_VOFS = 240;
  REG_BG0_VOFS = 256 - CHESS_BOARD_Y_OFS;
}
