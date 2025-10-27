#include "GBAdev_memdef.h"
#include "chess_ai_types.h"
#include "chess_board.h"
#include <GBAdev_functions.h>
#include <stdarg.h>
#include <stdio.h>

#define LSTRLEN(str) (sizeof(str)-1)

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
  ChessBoard_t tracking_board;
  u8 piece_hashmap[CHESS_BOARD_ROW_COUNT][CHESS_BOARD_FILE_COUNT];
  for (ChessBoard_File_e file=FILE_A; CHESS_BOARD_FILE_COUNT>file; ++file) {
    piece_hashmap[ROW_8][file] = BLACK_ROSTER_ID(file);
    piece_hashmap[ROW_7][file] = BLACK_ROSTER_ID(PAWN0|file);
    piece_hashmap[ROW_2][file] = WHITE_ROSTER_ID(PAWN0|file);
    piece_hashmap[ROW_1][file] = WHITE_ROSTER_ID(file);

    tracking_board[ROW_8][file] = BLACK_FLAGBIT|BOARD_BACK_ROWS_INIT[file];
    tracking_board[ROW_7][file] = BLACK_FLAGBIT|PAWN_IDX; 
    tracking_board[ROW_6][file] = EMPTY_IDX;
    tracking_board[ROW_5][file] = EMPTY_IDX;
    tracking_board[ROW_4][file] = EMPTY_IDX;
    tracking_board[ROW_3][file] = EMPTY_IDX;
    tracking_board[ROW_2][file] = WHITE_FLAGBIT|PAWN_IDX;
    tracking_board[ROW_1][file] = WHITE_FLAGBIT|BOARD_BACK_ROWS_INIT[file];
  }
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
