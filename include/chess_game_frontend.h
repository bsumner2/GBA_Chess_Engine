/** (C) 26 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_GAME_FRONTEND_
#define _CHESS_GAME_FRONTEND_

#include "chess_board.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#include <GBAdev_functions.h>
#include "subpixel.h"

// This macro + assertion of macro is basically my lazy way of writing front 
// end code to tell user that this feature is yet to be implemented
#define CPU_X_CPU_IMPLEMENTED FALSE
TODO("Implement CPU X CPU mode")

typedef enum e_game_mode_sel {
  GAME_MODE_2PLAYER=0,
  GAME_MODE_1PLAYER_V_CPU,
  GAME_MODE_CPU_V_1PLAYER,
  GAME_MODE_CPU_V_CPU
} GameModeSelection_e;

#define TOSTRSTR(l) #l
#define TOSTR(m) TOSTRSTR(m)

#define SELECT_CLR 0x4167
#define NORMAL_CLR 0x1484

#define AFFIRMITIVE_CLR 0x09C1
#define NEGATIVE_CLR 0x14CE

#define LSTRLEN(literal) (sizeof(literal)-1)

#define SEL_COUNT 4
#define SEL_MASK 3
#define SUBPIXEL_FONT_TEXT_HPOS_CENTERED(textlen)\
  ((M3_SCREEN_WIDTH-(textlen)*SubPixel_Glyph_Width)/2)

#define SUBPIXEL_FONT_TEXT_VPOS_CENTERED\
  ((M3_SCREEN_HEIGHT-SubPixel_Glyph_Height)/2)

#define M3_CLR_SCREEN()\
  Fast_Memset32(VRAM_M3,\
                  0,\
                  sizeof(u16)*M3_SCREEN_HEIGHT*M3_SCREEN_WIDTH/sizeof(WORD))

int M3FE_SelGamemode(void);

void ChessMoveHistory_Save(const ChessGameCtx_t *ctx);

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_GAME_FRONTEND_ */
