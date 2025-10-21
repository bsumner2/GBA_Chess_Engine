#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#include "chess_move_iterator.h"
#include "chess_move_iterator_block_allocator.h"
#include "chess_ai.h"
typedef union u_move_iteration_bufferent {
  struct s_chess_move_iteration_bufferent_hdr {
    u32 size;
    union u_move_iteration_bufferent *next_block;
  } header_block;
  ChessMoveIteration_t data_block;
} MoveIteration_Buffer_Heap_Entry_t;

struct s_chess_move_iterator_private {
  u32 cur_move;
  ChessMoveIteration_t *moves;
};

#define NALLOC 32

#define MAX_MOVE_CANDIDATES 27
#define MAX_ALLOCATABLE_BUFFERS (MAX_DEPTH*5 / 3)
#define FULL_BUFFER_ENTRY_NMEMB (MAX_MOVE_CANDIDATES + 1)
#define BUFFER_ENTRY_COUNT\
  (FULL_BUFFER_ENTRY_NMEMB*MAX_ALLOCATABLE_BUFFERS)

static_assert(0==(BUFFER_ENTRY_COUNT%NALLOC));

static EWRAM_BSS MoveIteration_Buffer_Heap_Entry_t 
                                      _L_priv_move_buffers[BUFFER_ENTRY_COUNT];
static IWRAM_BSS ChessMoveIterator_PrivateFields_t 
                              _L_priv_data_ents[MAX_ALLOCATABLE_BUFFERS];
static IWRAM_BSS BOOL 
               _L_priv_data_ents_occupied[MAX_ALLOCATABLE_BUFFERS] = {0};

static MoveIteration_Buffer_Heap_Entry_t 
                *_L_unused_page_break = &_L_priv_move_buffers[BUFFER_ENTRY_COUNT];
static MoveIteration_Buffer_Heap_Entry_t _L_base;
static MoveIteration_Buffer_Heap_Entry_t *_L_freep = NULL;

static MoveIteration_Buffer_Heap_Entry_t *MoreCore(size_t nalloc) {
  if (nalloc < NALLOC)
    nalloc = NALLOC;
  MoveIteration_Buffer_Heap_Entry_t *next_break=&_L_unused_page_break[-NALLOC];
  if (next_break < _L_priv_move_buffers)
    return NULL;
  next_break->header_block.size = nalloc; 
  MoveBufferHeap_Dealloc(&next_break[1].data_block);
  return _L_freep;
}

ChessMoveIterator_PrivateFields_t *MoveIterator_PrivateFields_Allocate(void) {
  for (int i = 0; MAX_ALLOCATABLE_BUFFERS > i; ++i) {
    if (_L_priv_data_ents_occupied[i])
      continue;
    _L_priv_data_ents_occupied[i] = TRUE;
    return &_L_priv_data_ents[i];
  }
  return NULL;
}

BOOL MoveIterator_PrivateFields_Deallocate(
                                      ChessMoveIterator_PrivateFields_t *obj) {
  if (((const uptr_t)obj<(uptr_t)_L_priv_data_ents) ||
      ((const uptr_t)&_L_priv_data_ents[MAX_ALLOCATABLE_BUFFERS-1]<(uptr_t)obj))
    return FALSE;

  ptrdiff_t diff = (ptrdiff_t)((uintptr_t)obj - (uintptr_t)_L_priv_data_ents);
  if (0!=(diff%sizeof(ChessMoveIterator_PrivateFields_t)))
    return FALSE;
  diff/=sizeof(ChessMoveIterator_PrivateFields_t);
  if (0 > diff || MAX_ALLOCATABLE_BUFFERS<=diff)
    return FALSE;
  if (&_L_priv_data_ents[diff]!=obj)
    return FALSE;
  obj->moves = NULL;
  obj->cur_move = 0;
  if (!_L_priv_data_ents_occupied[diff])
    return FALSE;
  _L_priv_data_ents_occupied[diff] = FALSE;
  return TRUE;
}



ChessMoveIteration_t *MoveBufferHeap_Alloc(size_t unit_count) {
  MoveIteration_Buffer_Heap_Entry_t *p, *prevp;
  if (!unit_count)
    return NULL;
  ++unit_count;
  if (NULL==(prevp=_L_freep)) {
    _L_base = (MoveIteration_Buffer_Heap_Entry_t){
      .header_block = {
        .size = 0,
        .next_block = _L_freep = prevp = &_L_base
      }
    };
  }
  for (p = prevp->header_block.next_block
      ; ;
      prevp = p, p = p->header_block.next_block) {
    if (p->header_block.size >= unit_count) {
      if (p->header_block.size==unit_count) {
        prevp->header_block.next_block = p->header_block.next_block;
      } else {
        p->header_block.size -= unit_count;
        p+=p->header_block.size;
        p->header_block.size = unit_count;
      }
      _L_freep = prevp;
      return (ChessMoveIteration_t*)&p[1].data_block;
    }
    if (p==_L_freep) {
      if (NULL==(p = MoreCore(unit_count)))
          return NULL;
    }
  }
}


void MoveBufferHeap_Dealloc(ChessMoveIteration_t *obj) {
  MoveIteration_Buffer_Heap_Entry_t *bp, *p;
  bp = &((MoveIteration_Buffer_Heap_Entry_t*)obj)[-1];
  for (p = _L_freep;
       bp <= p || bp >= p->header_block.next_block;
       p = p->header_block.next_block)
    if (p >= p->header_block.next_block 
        && (bp > p || bp < p->header_block.next_block))
      break;

  // If block we are freeing is contiguous with the p's upper neighbor, then
  // join free block to p's upper neighbor, and adjust pointers for p and
  // freeing block, bp.
  if (bp+bp->header_block.size == p->header_block.next_block) {
    bp->header_block.size 
      += p->header_block.next_block->header_block.size;
    bp->header_block.next_block 
      = p->header_block.next_block->header_block.next_block;
  } else {
    // otherwise, just make freeing block's (bp's) next ptr point to 
    // p's upper neighbor block
    bp->header_block.next_block = p->header_block.next_block;
  }
  if (p+p->header_block.size==bp) {
    p->header_block.size += bp->header_block.size;
    p->header_block.next_block = bp->header_block.next_block;
  } else {
    p->header_block.next_block = bp;
  }
  _L_freep = p;
}
