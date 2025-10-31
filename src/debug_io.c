#include "debug_io.h"
#include <stdarg.h>
#include <GBAdev_memmap.h>
#include <stdio.h>
#include <GBAdev_types.h>
#include "GBAdev_memdef.h"
#include "chess_gameloop.h"
#include "mode3_io.h"
#include "debug_io.h"

extern void IRQ_Sync(u32 flags);

const char *DebugIO_ChessPiece_ToString(ChessPiece_e piece) {
  switch (piece) {
  case WHITE_FLAGBIT:
    return "WHITE_PAWN";
  case BLACK_FLAGBIT:
    return "BLACK_PAWN";
  case BLACK_BISHOP:
    return "BLACK_BISHOP";
  case BLACK_ROOK:
    return "BLACK_ROOK";
  case BLACK_KNIGHT:
    return "BLACK_KNIGHT";
  case BLACK_QUEEN:
    return "BLACK_QUEEN";
  case BLACK_KING:
    return "BLACK_KING";
  case WHITE_BISHOP:
    return "WHITE_BISHOP";
  case WHITE_ROOK:
    return "WHITE_ROOK";
  case WHITE_KNIGHT:
    return "WHITE_KNIGHT";
  case WHITE_QUEEN:
    return "WHITE_QUEEN";
  case WHITE_KING:
    return "WHITE_KING";
  default:
    return "INVALID_VALUE";
  }
}
  
__attribute__ (( __format__ ( __printf__, 3, 4 ), __noreturn__ ))
void Debug_PrintfAndExitInternal(const char *__restrict func, u32 line,
                                const char *__restrict fmt, ...) {
                                          
  Fast_Memset32(VRAM_M3, 
                0, 
                sizeof(u16)*M3_SCREEN_WIDTH*M3_SCREEN_HEIGHT/sizeof(WORD));
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);


  va_list args;
  i32 len;
  va_start(args, fmt);
  len = vsnprintf(NULL,0, fmt, args);
  va_end(args);
  assert(0 < len);
  char *output_string = calloc(len+1, sizeof(char));
  va_start(args, fmt);
  vsnprintf(output_string, len+1, fmt, args);
  va_end(args);
  mode3_printf(0,0, ERR_LABEL_CLR, 
               "[Error @ %s:%lu]:\x1b[" TO_EXP_STR(DEF_ERR_MSG_CLR) "] %s",
               func,
               line,
               output_string);
  free(output_string);
  do {
    IRQ_Sync(1<<IRQ_KEYPAD);
    exit(1);
  } while (1);
}

PRINTF_LIKE(3, 4) void Debug_PrintfInternal(const char *__restrict func,
                                                               u32 line,
                                             const char *__restrict fmt,
                                                                    ...) {
  Fast_Memset32(VRAM_M3, 
                0, 
                sizeof(u16)*M3_SCREEN_WIDTH*M3_SCREEN_HEIGHT/sizeof(WORD));
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);

  va_list args;
  char *outstr;
  i32 len;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  assert(0 < len);
  outstr = calloc(len+1, sizeof(char));
  va_start(args, fmt);
  vsnprintf(outstr, len+1, fmt, args);
  va_end(args);
  mode3_printf(0,0, ERR_LABEL_CLR, 
               "[Error @ %s:%lu]:\x1b[" TO_EXP_STR(DEF_ERR_MSG_CLR) "] %s",
               func,
               line,
               outstr);
  free(outstr);
}

