/** (C) 19 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _DEBUG_IO_
#define _DEBUG_IO_

#include "chess_board.h"
#include "mode3_io.h"
#include <GBAdev_types.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */


#define DEF_ERR_MSG_CLR       0x1484
#define ERR_LABEL_CLR 0x082F
#define TO_STR(txt) #txt
#define TO_EXP_STR(macro) TO_STR(macro)

#define DBIO_DATA_PRINT_CLR(flag, color)\
  TXT_IO_COLOR_FMT(color) "%" flag "\x1b[" TO_EXP_STR(DEF_ERR_MSG_CLR) "]"

#define DEBUG_MSG_EXITS 1
#define DEBUG_MSG_RETURNS 0


#define ensure(expr, debug_msg_fmt, ...) ((expr) ? ((void)0)\
    : Debug_PrintfAndExitInternal(__PRETTY_FUNCTION__, __LINE__,\
          "Ensure expression, \x1b[0x44E4](" #expr ")\x1b[0x1484], failed.\n"\
          "Details: "debug_msg_fmt, __VA_ARGS__))

#define DebugMsgF(no_return, fmt, ...)\
  (no_return ? Debug_PrintfAndExitInternal(__PRETTY_FUNCTION__, __LINE__, fmt, __VA_ARGS__)\
            : Debug_PrintfInternal(__PRETTY_FUNCTION__, __LINE__, fmt, __VA_ARGS__))

#define DebugMsg(no_return, s)\
  (no_return ? Debug_PrintfAndExitInternal(__PRETTY_FUNCTION__, __LINE__, s)\
             : Debug_PrintfInternal(__PRETTY_FUNCTION__, __LINE__, s))


#define ENSURE_STACK_SAFETY()  \
  {  \
     _Pragma("GCC diagnostic push")  \
    DIAGNOSTICS_SUPPRESS(-Wuninitialized)  \
    extern const void __iwram_overlay_end;  \
    register void *sp __asm("sp");  \
    ensure(sp > &__iwram_overlay_end,  \
        "\n\tsp=\x1b[0x44E4]%p\x1b[0x1484]\n\tEnd of stack=\x1b[0x44E4]%p",  \
        sp,  \
        (void*)&__iwram_overlay_end);  \
    _Pragma("GCC diagnostic pop")\
  }

const char *DebugIO_ChessPiece_ToString(ChessPiece_e piece);
__attribute__ (( __format__ ( __printf__, 3, 4 ), __noreturn__ )) 
void Debug_PrintfAndExitInternal(const char *__restrict func, u32 line,
                                 const char *__restrict fmt, ...);

PRINTF_LIKE(3,4) void Debug_PrintfInternal(const char *__restrict funct,
                                                               u32 line,
                                             const char *__restrict fmt,
                                                                   ...);



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _DEBUG_IO_ */
