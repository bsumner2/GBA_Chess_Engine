/** (C) 16 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include "chess_move_iterator.h"
#include <GBAdev_functions.h>
#include "chess_board.h"
#include "chess_board_state.h"
struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};
struct s_chess_move_iterator_private {
  u32 cur_move;
  ChessMoveIteration_t *moves;
};

// Theres probly a more space-efficient value for max, but fuck it.
#define MAX_MOVE_CANDIDATES 64
static EWRAM_BSS ChessMoveIteration_t _L_move_buffer[MAX_MOVE_CANDIDATES];
// We can do this thanks to GBA being single-threaded.
static const EWRAM_BSS BoardState_t *_L_cur_board_state=NULL;
static EWRAM_DATA ChessBoard_Idx_t _L_cur_piece_location = INVALID_IDX;

typedef struct s_INTERNAL_chess_move_iterator InternalMoveIterator_t;
struct s_INTERNAL_chess_move_iterator {
  Mvmt_Dir_e directions[8];
  ChessBoard_Idx_t base, curmove;
  union u_gp_vals {
    u16 cur_dir_idx;
    struct s_king_special_use {
      u16 cur_dir_idx;
      u8 castle_moves_tried;  /// ACTIVE LOW, so:
                               /// bit(x) HIGH -> castle_mv(x) not yet iter'd
      u8 cur_castle_move;
    } king_vals;
    struct s_pawn_special_use {
      u16 cur_dir_idx;
      u16 cur_promo_type;
    } pawn_vals;
  } gp_vals;
  ChessPiece_e piece;
};


extern InternalMoveIterator_t *InternalMoveIterator_Init(
                                           InternalMoveIterator_t *iterator,
                                           ChessBoard_Idx_t start_loc,
                                           ChessPiece_e piece_type);
extern void InternalMoveIterator_Uninit(InternalMoveIterator_t *iterator);

extern BOOL InternalMoveIterator_HasNext(InternalMoveIterator_t *iterator);
extern BOOL InternalMoveIterator_Next(InternalMoveIterator_t *iterator,
                                      ChessMoveIteration_t *dest);
extern BOOL InternalMoveIterator_IsContinuousMovementIterator(
                                       const InternalMoveIterator_t *iterator);
extern BOOL InternalMoveIterator_ContinuousForceNextDirection(
                                              InternalMoveIterator_t *iterator);

static int Capture_Eval(ChessBoard_Idx_t loc);
static int Knight_Move_Eval(ChessBoard_Idx_t loc);
static int Promo_Flag_Eval(int promo_flag);
static int __MoveIterationCmp(const void *a, const void *b);

int Capture_Eval(ChessBoard_Idx_t loc) {
  PieceState_Graph_Vertex_t capt_graphnode;
  capt_graphnode 
    = _L_cur_board_state
              ->graph.vertices[_L_cur_board_state->graph.vertex_hashmap[
                                                              BOARD_IDX(loc)]];
  return 2*capt_graphnode.attacking_count 
                - capt_graphnode.defending_count;
}

int Knight_Move_Eval(ChessBoard_Idx_t loc) {
  int move_count=0;
  BOOL can_up_tall, can_up_wide, can_down_tall, can_down_wide;
  can_up_wide = can_up_tall = ROW_7 < loc.coord.y;
  // can_up_tall -> can_up_wide, so, unless !(can_up_wide=can_up_tall=...),
  // can_up_wide is valid, and doesnt need to be set again.
  if (!can_up_wide)
    can_up_wide = ROW_8 < loc.coord.y;
  can_down_wide = can_down_tall = ROW_2 > loc.coord.y;
  // can_down_tall -> can_down_wide, so same logic here.
  if (!can_down_wide)
    can_down_wide = ROW_1 > loc.coord.y;
  if (FILE_A<loc.coord.x) {
    if (can_up_tall) {
      ++move_count;
      // because if can up wide is necessary for can_up_tall, we can just check
      // to see if left wide
      if (FILE_B < loc.coord.x)
        ++move_count;
    } else if (FILE_B<loc.coord.x) {
      if (can_up_wide)
        ++move_count;
    }
    if (can_down_tall) {
      ++move_count;
      if (FILE_B < loc.coord.x) {
        ++move_count;
      }
    } else if (can_down_wide) {
      if (can_down_wide)
        ++move_count;
    }
  }
  if (FILE_H > loc.coord.x) {
    if (can_up_tall) {
      ++move_count;
      if (FILE_G > loc.coord.x)
        ++move_count;
    } else if (can_up_wide) {
      if (FILE_G > loc.coord.x)
        ++move_count;
    }
    if (can_down_tall) {
      ++move_count;
      if (FILE_G > loc.coord.x)
        ++move_count;
    } else if (can_down_wide) {
      if (FILE_G > loc.coord.x)
        ++move_count;
    }
  }
  return move_count;
}

int Promo_Flag_Eval(int promo_flag) {
  if (!promo_flag)
    return 0;
  switch ((ChessPiece_e)promo_flag) {
  case QUEEN_IDX:
    return 4;
  case ROOK_IDX:
    return 3;
  case BISHOP_IDX:
    return 2;
  case KNIGHT_IDX:
    return 1;
  default:
    assert(0);
    return ~0;
  }
}


int __MoveIterationCmp(const void *a, const void *b) {
  const ChessMoveIteration_t *lhs=a, *rhs=b;
  assert(_L_cur_board_state!=NULL);
  ChessPiece_e start_piece, lhs_dst_piece, rhs_dst_piece;
  BOOL lhs_empty;
  lhs_dst_piece = _L_cur_board_state->board[BOARD_IDX(lhs->dst)];
  rhs_dst_piece = _L_cur_board_state->board[BOARD_IDX(rhs->dst)];

  assert(INVALID_IDX_RAW_VAL == _L_cur_piece_location.raw);
  start_piece = PIECE_IDX_MASK
                  & _L_cur_board_state->board[BOARD_IDX(_L_cur_piece_location)];

  lhs_empty = (EMPTY_IDX==lhs_dst_piece);
  if (lhs_empty^(EMPTY_IDX==rhs_dst_piece)) {  // 1 operand empty, other not
    return (EMPTY_IDX==lhs_dst_piece) ? -1 : 1;  // and if lhs is the empty 1,
                                                 // negative cmp return
  }
  if (lhs_empty) {
    // if both spots equal, just evaluate based on distance for cmp, OR
    // (special cases: castle, promo, and knight moving into edge or corner

    // Special Cases:
    if (KNIGHT_IDX==start_piece) {
      return Knight_Move_Eval(lhs->dst) - Knight_Move_Eval(rhs->dst);
    } else if (KING_IDX==start_piece || PAWN_IDX==start_piece) {
      if (PAWN_IDX==start_piece) {
        if (lhs->promotion_flag != rhs->promotion_flag) {
          // THis works because default value of ZERO means no promo occurred,
          // and then promotion_flag
          return Promo_Flag_Eval(lhs->promotion_flag) 
                      - Promo_Flag_Eval(rhs->promotion_flag);
        }
      }
      if (MOVE_SPECIAL_MOVE_FLAGS_MASK&lhs->special_flags) {
        return MOVE_SPECIAL_MOVE_FLAGS_MASK&rhs->special_flags ? 0 : 1;
      } else if (MOVE_SPECIAL_MOVE_FLAGS_MASK&rhs->special_flags) {
        return -1;
      }
    }
    
    // Distance based scoring
    int dx,dy, lhs_dist_magnitude, rhs_dist_magnitude;
    dx = lhs->dst.arithmetic.x - _L_cur_piece_location.arithmetic.x;
    dy = lhs->dst.arithmetic.y - _L_cur_piece_location.arithmetic.y;
    lhs_dist_magnitude = dx*dx + dy*dy;

    dx = rhs->dst.arithmetic.x - _L_cur_piece_location.arithmetic.x;
    dy = rhs->dst.arithmetic.y - _L_cur_piece_location.arithmetic.y;
    rhs_dist_magnitude = dx*dx + dy*dy;
    assert(lhs_dist_magnitude>0 && rhs_dist_magnitude>0);
    return lhs_dist_magnitude - rhs_dist_magnitude;

  }
  return Capture_Eval(lhs->dst) - Capture_Eval(rhs->dst);

  
  
  
}
BOOL ChessMoveIterator_Alloc(ChessMoveIterator_t *dst_iterator,
                             ChessBoard_Idx_t piece_location,
                             const BoardState_t *state,
                             ChessMoveIterator_MoveSetMode_e mode) {

  InternalMoveIterator_t iter;
  ChessMoveIterator_t iterator={0};
  ChessMoveIteration_t cur_mv;
  const ChessPiece_e (*BOARD_DATA)[CHESS_BOARD_ROW_COUNT] = state->board;
  ChessPiece_e mv_piece = BOARD_DATA[BOARD_IDX(piece_location)], curpiece;
  const u32 ALLIED_TEAM_FLAGBIT = mv_piece&PIECE_TEAM_MASK,
            OPP_TEAM_FLAGBIT = PIECE_TEAM_MASK^ALLIED_TEAM_FLAGBIT;
  u32 count = 0;
  BOOL order = MV_ITER_MOVESET_ORDERED(mode);
  mode = MV_ITER_MOVESET_SET_TYPE(mode);
  Fast_Memset32(&iter, 0, sizeof(InternalMoveIterator_t)/sizeof(WORD));
  *dst_iterator = iterator;
  if (NULL == InternalMoveIterator_Init(&iter,
                                      piece_location,
                                      mv_piece))
    return FALSE;
  static_assert((0==(MAX_MOVE_CANDIDATES*sizeof(ChessMoveIteration_t))%sizeof(WORD)));
  Fast_Memset32(_L_move_buffer, 0, 
      MAX_MOVE_CANDIDATES*sizeof(ChessMoveIteration_t)/sizeof(WORD));
  while (InternalMoveIterator_HasNext(&iter)) {
    assert(InternalMoveIterator_Next(&iter, &cur_mv));
    curpiece = BOARD_DATA[BOARD_IDX(cur_mv.dst)];
    switch (mode) {
    case MV_ITER_MOVESET_ALL_MINUS_ALLIED_COLLISIONS:
      if (EMPTY_IDX==curpiece) {
        _L_move_buffer[count++] = cur_mv;
        continue;
      }
      assert(curpiece&PIECE_TEAM_MASK);
      if (InternalMoveIterator_IsContinuousMovementIterator(&iter))
        assert(InternalMoveIterator_ContinuousForceNextDirection(&iter));
      if (curpiece&ALLIED_TEAM_FLAGBIT) {
        continue;
      } else {
        cur_mv.special_flags |= MOVE_CAPTURE;
        _L_move_buffer[count++] = cur_mv;
        continue;
      }
      break;
    case MV_ITER_MOVESET_COLLISIONS_ONLY_SET:
      if (EMPTY_IDX==curpiece) {
        continue;
      }
      assert(curpiece&PIECE_TEAM_MASK);
      if (InternalMoveIterator_IsContinuousMovementIterator(&iter))
        assert(InternalMoveIterator_ContinuousForceNextDirection(&iter));
      if (OPP_TEAM_FLAGBIT&curpiece)
        cur_mv.special_flags|=MOVE_CAPTURE;
      _L_move_buffer[count++] = cur_mv;
      continue;
      break;
    case MV_ITER_MOVESET_ALL_SET:
      if (EMPTY_IDX==curpiece) {
        _L_move_buffer[count++] = cur_mv;
        continue;
      }
      assert(curpiece&PIECE_TEAM_MASK);
      if (InternalMoveIterator_IsContinuousMovementIterator(&iter))
        assert(InternalMoveIterator_ContinuousForceNextDirection(&iter));
      if (OPP_TEAM_FLAGBIT&curpiece)
        cur_mv.special_flags |= MOVE_CAPTURE;
      break;
    default:
      assert(0);
    }
  }
  iterator.priv = malloc(sizeof(ChessMoveIterator_PrivateFields_t));
  iterator.priv->moves = malloc(sizeof(ChessMoveIteration_t)*count);
  static_assert(0==(sizeof(ChessMoveIteration_t)%sizeof(WORD)));
  Fast_Memcpy32(iterator.priv->moves,
                _L_move_buffer,
                count*sizeof(ChessMoveIteration_t)/sizeof(WORD));
  // Need to pass current state to src file local, _L_cur_board_state
  _L_cur_board_state = state;
  _L_cur_piece_location = piece_location;
  qsort(iterator.priv->moves,
        count,
        sizeof(ChessMoveIteration_t),
        __MoveIterationCmp);
  iterator.size = count;
  *dst_iterator = iterator;
  return TRUE;
}

BOOL ChessMoveIterator_HasNext(const ChessMoveIterator_t *iterator) {
  if (NULL==iterator)
    return FALSE;
  if (NULL==iterator->priv)
    return FALSE;
  if (NULL==iterator->priv->moves)
    return FALSE;
  if (0==iterator->size)
    return FALSE;
  return iterator->size > iterator->priv->cur_move;
}

BOOL ChessMoveIterator_Next(ChessMoveIterator_t *iterator,
                            ChessMoveIteration_t *ret_mv) {
  if (!ChessMoveIterator_HasNext(iterator))
    return FALSE;
  if (NULL==ret_mv)
    return FALSE;
  ChessMoveIteration_t *moves = iterator->priv->moves;
  *ret_mv = moves[++iterator->priv->cur_move];
  return TRUE;
}

BOOL ChessMoveIterator_Dealloc(ChessMoveIterator_t *iterator) {
  if (NULL==iterator)
    return FALSE;
  if (NULL==iterator->priv)
    return FALSE;
  if (NULL==iterator->priv->moves)
    return FALSE;
  if (0==iterator->size)
    return FALSE;
  free(iterator->priv->moves);
  free(iterator->priv);
  static_assert(sizeof(ChessMoveIterator_t)==sizeof(u64));
  *((u64*)iterator) = 0ULL;
  assert(0==iterator->size && NULL==iterator->priv);
  return TRUE;
}
