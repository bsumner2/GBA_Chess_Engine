/** (C) 27 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _KEY_STATUS_
#define _KEY_STATUS_

#include <GBAdev_types.h>
#include <GBAdev_memdef.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

extern u16 KEY_CURR, KEY_PREV;

//#define KEY_PRESSED(key) (KEY_##key&KEY_CURR)
//#define KEY_STROKE(key)  ((KEY_##key&KEY_CURR) && !(KEY_##key&KEY_PREV))

#define KEY_STROKE(key1, ...)\
  KEY_STROKE_INTERNAL((KEY_##key1 __VA_OPT__(|EXPAND(MULTI_KEY_HELPER(__VA_ARGS__)))))

#define KEY_PRESSED(key1, ...)\
  ((KEY_##key1 __VA_OPT__(|EXPAND(MULTI_KEY_HELPER(__VA_ARGS__))))&KEY_CURR)

#define MULTI_KEY_HELPER(key1, ...)\
  ((KEY_##key1)\
  __VA_OPT__(|MULTI_KEY_TERTIARY PARENTHESIS (__VA_ARGS__)))
#define MULTI_KEY_TERTIARY() MULTI_KEY_HELPER
#define KEY_STROKE_INTERNAL(keys)\
  ((keys&KEY_CURR) && !((keys&KEY_CURR)&KEY_PREV))
#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _KEY_STATUS_ */
