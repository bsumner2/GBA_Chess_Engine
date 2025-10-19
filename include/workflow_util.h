/** (C) 15 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _WORKFLOW_UTIL_
#define _WORKFLOW_UTIL_

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#include <GBAdev_util_macros.h>
#define __DEFAULT_CODE_SECTION__
#define __DEFAULT_SCOPE__

#define __FUNC_STUB__(prototype_or_implementation,\
                      scope,\
                      section,\
                      rtype,\
                      name,\
                      ...)\
  __FUNC_STUB_##prototype_or_implementation##__\
          (scope, section, rtype, name, __VA_ARGS__)
#define __FUNC_STUB_PROTOTYPE__(scope, section, rtype, name, ...)\
  TODO("IMPLEMENT FUNCTION STUB");\
  scope section rtype name(__VA_ARGS__)
#define __FUNC_STUB_IMPLEMENTATION__(scope, section, rtype, name, ...)\
  TODO("COMPLETE IMPLEMENTATION")\
  DO_PRAGMA(GCC diagnostic ignored "-Wunused-parameter")\
  scope section rtype name(__VA_ARGS__)
  



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _WORKFLOW_UTIL_ */
