/** (C) 21 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_MOVE_ITERATOR_BLOCK_ALLOCATOR_
#define _CHESS_MOVE_ITERATOR_BLOCK_ALLOCATOR_

#include "chess_move_iterator.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

IWRAM_CODE ChessMoveIterator_PrivateFields_t *MoveIterator_PrivateFields_Allocate(void);
IWRAM_CODE BOOL MoveIterator_PrivateFields_Deallocate(
                                      ChessMoveIterator_PrivateFields_t *obj);
IWRAM_CODE ChessMoveIteration_t *MoveBufferHeap_Alloc(size_t unit_count);
IWRAM_CODE void MoveBufferHeap_Dealloc(ChessMoveIteration_t *obj);


#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_MOVE_ITERATOR_BLOCK_ALLOCATOR_ */
