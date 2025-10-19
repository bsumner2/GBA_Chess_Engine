/** (C) 7 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _GBADEV_UTIL_MACROS_
#define _GBADEV_UTIL_MACROS_

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#define IWRAM_CODE __attribute__ (( section(".iwram"), long_call))
#define EWRAM_CODE __attribute__ (( section(".ewram"), long_call))

#define IWRAM_DATA __attribute__ (( section(".iwram") ))
#define IWRAM_BSS __attribute__ (( section(".bss") ))
#define EWRAM_DATA __attribute__ (( section(".ewram") ))
#define EWRAM_BSS __attribute__ (( section(".sbss") ))
#define ALIGN(alignment) __attribute__ (( aligned(alignment) ))
#define PACKED __attribute__ (( packed ))

#define DO_PRAGMA(pg) _Pragma (#pg)
#define TODO(msg) DO_PRAGMA(message("TODO: " #msg))

#define INLN static inline

#define ASM(...) __asm volatile (__VA_ARGS__)


#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _GBADEV_UTIL_MACROS_ */
