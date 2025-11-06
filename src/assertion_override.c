
#include "chess_gameloop.h"
#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <stdlib.h>
#define EXIT_PROCEDURE() exit(EXIT_FAILURE)
#include "mode3_io.h"
extern void Mode3_ClearScreen(u16 color);
void __assert_func(const char *src_filename, int line_no, const char *caller, const char *assertion) {
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BLANK);
  Mode3_ClearScreen(0);
  REG_DPY_CNT = REG_VALUE(DPY_CNT, MODE, 3)|REG_FLAG(DPY_CNT, BG2);
  mode3_printf(0, 0, 0x1069,
               "[Error]:\x1b[0x10A5] Assertion expression, \x1b[0x2483]"
               "%s"
               "\x1b[0x10A5], has \x1b[0x1069]failed.\x1b[0x8000]\n\t"
               "\x1b[0x2483]Assertion from Function: \x1b[0x7089]%s\x1b[0x8000]\n\t"
               "\x1b[0x2483]Defined In: \x1b[0x7089]%s\x1b[0x8000]\t\x1b[0x2483]Line No.: \x1b[0x7089]%d\n"
               , assertion, caller, src_filename, line_no);
  REG_IME = 0;
  REG_IF |= IRQ_FLAG(KEYPAD);
  REG_IE |= IRQ_FLAG(KEYPAD);
  Ksync(START, KSYNC_DISCRETE);
  Mode3_ClearScreen(0);
  EXIT_PROCEDURE();
}

