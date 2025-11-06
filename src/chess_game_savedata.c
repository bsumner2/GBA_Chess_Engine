/** 
 * (C) Burt O Sumner 2025 Authorship rights reserved.
 * Free to use so long as this credit comment remains visible here.
 **/

#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>
#include <stdarg.h>
#include <stdio.h>
#include <GBAdev_memmap.h>
#include "chess_board.h"
#include "chess_game_frontend.h"
#include "chess_gameloop.h"
#include "mode3_io.h"
#include "subpixel.h"


#define DEFAULT_PGN_TAGS\
  "[Event \"GBA Chess Game\"]\n"\
  "[Site \"GBA\"]\n"\
  "[Date \"2025\"]\n"\
  "[Round \"N/A\"]\n"



u32 write_cursor = 4;
extern BOOL SRAM_Fill(u8 fill_byte, size_t nbytes, uintptr_t ofs);

static u32 SRAM_printf(u32 print_ofs, const char *__restrict fmt, ...) {
  int len;
  va_list args;
  va_start(args, fmt);
  len = snprintf(NULL, 0, fmt, args);
  va_end(args);
  char buf[len+1];
  va_start(args, fmt);
  snprintf(buf, len+1, fmt, args);
  va_end(args);
  buf[len] = '\0';
  SRAM_Write(buf, len, print_ofs);
  return len;
}

static const char *Result_ToStr(u32 result) {
  switch (result) {
  case WHITE_FLAGBIT:
    return "1-0";
  case BLACK_FLAGBIT:
    return "0-1";
  case 0:
    return "1/2-1/2";
  }
  return "INVALID RESULT";
}


void PGN_Save(const char *__restrict opp1,
              const char *__restrict opp2,
              const PGN_Round_LL_t *moves,
              u32 last_to_move,
              u32 result) {
  u32 len = 0;
  {
    u32 clr_amt;
    SRAM_Read(&clr_amt, 4, SRAM_SIZE-4);
    if (clr_amt&(SRAM_SIZE - 1))
      assert(SRAM_Fill(0, clr_amt, 0));
  }
  len = LSTRLEN(DEFAULT_PGN_TAGS);
  SRAM_Write(DEFAULT_PGN_TAGS, LSTRLEN(DEFAULT_PGN_TAGS), 0);
  len+=SRAM_printf(len, 
                   "[White \"%s\"]\n[Black \"%s\"]\n"
                   "[Result \"%s\"]\n\n",
                   opp1, opp2, Result_ToStr(result));
  u32 round = 1;
  const PGN_Round_LL_Node_t *const 
          LAST = last_to_move&WHITE_FLAGBIT?moves->tail:NULL;
  for (const PGN_Round_LL_Node_t *cur=moves->head; LAST!=cur; cur=cur->next) {
    len+=SRAM_printf(len, "%d. ", round++);
    for (u32 i=0; 2>i; ++i) {
    }
  }
  SRAM_Write(&len, 4, SRAM_SIZE-4);
}

void ChessMoveHistory_Save(const ChessGameCtx_t *ctx) {
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);

  
  
  extern ChessGameCtx_t context;
  if (ctx->move_hist.nmemb==0) {
    mode3_printf(
        SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("   No Game Data To Save!  ")),
        SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
        0x10A5,
        "   No Game Data To Save!  \nPress \x1b[" TOSTR(AFFIRMITIVE_CLR) "]"
        "[START]\x1b[0x10A5] to continue!");
    Ksync(START, KSYNC_DISCRETE);
    M3_CLR_SCREEN();
    return;
  }
  mode3_printf(
      SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("Game Save in Progress!")),
      SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
      0x10A5,
      "Game Save in Progress!");
  mode3_printf(
      SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("Please DO NOT Remove Cartridge"
                                               " While Saving Game!")),
      SUBPIXEL_FONT_TEXT_VPOS_CENTERED+2*SubPixel_Glyph_Height,
      0x10A5,
      "Please \x1b[" TOSTR(NEGATIVE_CLR) "]DO NOT\x1b[0x10A5] "
      "Remove Cartridge While Saving Game!");

  assert(SRAM_Write(&ctx->whose_turn, 4, 4));
  
  u32 sz;
  sz = ctx->move_hist.nmemb * sizeof(PGN_Round_t);

  assert(SRAM_Write(&sz, 4, 0));
  sz = 8;

  for (const PGN_Round_LL_Node_t *node = ctx->move_hist.head;
       NULL!=node;
       node = node->next) {
    assert(SRAM_Write(&node->data, sizeof(PGN_Round_t), sz));
    sz+=sizeof(PGN_Round_t);
  }
  M3_CLR_SCREEN();
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("Game Saved! Press [ST"
                                                        "ART] to continue!")),
      SUBPIXEL_FONT_TEXT_VPOS_CENTERED,
      0x10A5, "Game Saved! Press \x1b[" TOSTR(AFFIRMITIVE_CLR) "][START]"
      "\x1b[0x10A5] to continue!");
  Ksync(START, KSYNC_DISCRETE);
  M3_CLR_SCREEN();
  return;

}

