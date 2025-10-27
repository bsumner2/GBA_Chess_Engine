/** (C) 18 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_OBJ_SPRITES_DATA_
#define _CHESS_OBJ_SPRITES_DATA_
#include <GBAdev_memdef.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#define CHESS_OATTR_SHAPE_VALUE OBJ_SHAPE_SQUARE
#define CHESS_OATTR_SIZE_VALUE  OBJ_SIZE_VALUE(SQUARE, 32, 32)
#define CHESS_SPRITE_DIMS       32
#define CSPR_TILES_PER_DIM      (CHESS_SPRITE_DIMS/TILE8_DIMS)
#define TILES_PER_CSPR          (CSPR_TILES_PER_DIM*CSPR_TILES_PER_DIM)

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_OBJ_SPRITES_DATA_ */
