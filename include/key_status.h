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

#define KEY_PRESSED(key) (KEY_##key&KEY_CURR)
#define KEY_STROKE(key)  ((KEY_##key&KEY_CURR) && !(KEY_##key&KEY_PREV))

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _KEY_STATUS_ */
