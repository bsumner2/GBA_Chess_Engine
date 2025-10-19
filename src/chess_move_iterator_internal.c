/** (C) 11 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#include <GBAdev_functions.h>
#include "chess_board.h"
#include "chess_board_state.h"
#include "chess_move_iterator.h"



#define PAWN_PROMOTION_TYPE_CT 4

extern const ChessPiece_e PROMOTION_SEL[4];

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

typedef enum e_piece_mv_dirs_count {
  PAWN_MVMT_CT=4,
  QUEEN_MVMT_CT=8,
  KNIGHT_MVMT_CT=8,
KING_MVMT_CT=8,
  ROOK_MVMT_CT=4,
  BISHOP_MVMT_CT=4,
} ChessPiece_Mvmt_Dir_Count_e;

#define BISHOP_OR_ROOK_MVMT_COUNT 4

static BOOL ChessMove_DiscreteMoveDirIterator_HasNext(
                                              InternalMoveIterator_t *iterator);

static BOOL ChessMove_ContinuousMoveDirIterator_HasNext(
                                              InternalMoveIterator_t *iterator);
static BOOL InternalMoveIterator_GetContinuous(ChessBoard_Idx_t *query_idx,
                                               Mvmt_Dir_e dir);
static BOOL ChessMove_KingMoveDirIterator_HasNext(
                                          InternalMoveIterator_t *iterator);
static void InternalMoveIterator_ApplyPawnMove(
                                            InternalMoveIterator_t *iterator,
                                            ChessMoveIteration_t *dest);
static void InternalMoveIterator_ApplyKingMove(
                                            InternalMoveIterator_t *iterator,
                                            ChessMoveIteration_t *dest);
static void InternalMoveIterator_ApplyKnightMove(
                                              InternalMoveIterator_t *iterator,
                                              ChessMoveIteration_t *dest);
static void InternalMoveIterator_ApplyContinuousMove(
                                              InternalMoveIterator_t *iterator,
                                              ChessMoveIteration_t *dest);
static int InternalMoveIterator_GetDirectionCount(
                                        const InternalMoveIterator_t *iterator);


void InternalMoveIterator_Uninit(InternalMoveIterator_t *iterator) {
  Fast_Memset32(iterator, 0, sizeof(InternalMoveIterator_t)/sizeof(WORD));
}

InternalMoveIterator_t *InternalMoveIterator_Init(
                                            InternalMoveIterator_t *iterator,
                                            ChessBoard_Idx_t start_loc,
                                            ChessPiece_e piece_type) {
  if (NULL==iterator)
    return NULL;
  Mvmt_Dir_e *dirs;
  u32 dir_count = 0;
  BOOL invalid_directions_init_state;
  iterator->base = iterator->curmove = start_loc;
  iterator->piece = piece_type;
  iterator->gp_vals.cur_dir_idx = 0;
  dirs = iterator->directions;
  if (PAWN_IDX==(PIECE_IDX_MASK&piece_type)) {
    const ChessBoard_Row_e DOUBLE_SQR_ROW
      = (WHITE_FLAGBIT&piece_type) ? ROW_4 : ROW_5;
    const Mvmt_Dir_e VERT_FLAGBIT
      = (WHITE_FLAGBIT&piece_type) ? UP_FLAGBIT : DOWN_FLAGBIT;

    iterator->gp_vals.pawn_vals.cur_promo_type = 0;

    assert(((WHITE_FLAGBIT&piece_type) ? ROW_8 : ROW_1)!=start_loc.coord.y);
    dir_count = PAWN_MVMT_CT;
    if (DOUBLE_SQR_ROW==start_loc.coord.y) {
      dirs[2] = VERT_FLAGBIT;
    }
    dirs[1] = VERT_FLAGBIT;
    if (FILE_A < start_loc.coord.x)
      dirs[0] = VERT_FLAGBIT|LEFT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
    if (FILE_H > start_loc.coord.x)
      dirs[3] = VERT_FLAGBIT|RIGHT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
    for (int i=0; PAWN_MVMT_CT>i; ++i) {
      if (dirs[i])
        continue;
      dirs[i] = INVALID_MVMT_FLAGBIT;
    }
  } else if (KNIGHT_MVMT_FLAGBIT==(PIECE_IDX_MASK&piece_type)) {
    dir_count = KNIGHT_MVMT_CT;
    if (FILE_B < start_loc.coord.x) {
      // tall left moves, AND...
      dirs[0] = dirs[3] = LEFT_FLAGBIT;
      // ... wide left moves
      dirs[1] = dirs[2] = MVMT_WIDE_FLAGBIT|LEFT_FLAGBIT;
    } else if (FILE_A < start_loc.coord.x) {
      // tall left moves only
      dirs[0] = dirs[3] = LEFT_FLAGBIT;
    }
    if (FILE_G > start_loc.coord.x) {
      // tall right moves, AND...
      dirs[4] = dirs[7] = RIGHT_FLAGBIT;
      // ... wide right moves
      dirs[5] = dirs[6] = MVMT_WIDE_FLAGBIT|RIGHT_FLAGBIT;
    } else if (FILE_H > start_loc.coord.x) {
      // tall right moves only
      dirs[4] = dirs[7] = RIGHT_FLAGBIT;
    }
    // tall moves are 0,3 and 4,7 for left up, left down, right up, right down,
    // respectively.
    // wide moves are 1,2 and 5,6 for left up, left down, right up, right down
    if (ROW_7 < start_loc.coord.y) {
      // tall up moves, AND...
      dirs[0] |= UP_FLAGBIT, dirs[4] |= UP_FLAGBIT;
      // ... wide up moves
      dirs[1] |= UP_FLAGBIT, dirs[5] |= UP_FLAGBIT;
    } else if (ROW_8 < start_loc.coord.y) {
      // tall up moves only
      dirs[0] |= UP_FLAGBIT, dirs[4] |= UP_FLAGBIT;
    }
    if (ROW_2 > start_loc.coord.y) {
      // tall down moves, AND...
      dirs[3] |= DOWN_FLAGBIT, dirs[7] |= DOWN_FLAGBIT;
      // ... wide down moves
      dirs[2] |= DOWN_FLAGBIT, dirs[6] |= DOWN_FLAGBIT;
    } else if (ROW_1 > start_loc.coord.y) {
      // tall down moves only
      dirs[3] |= DOWN_FLAGBIT, dirs[7] |= DOWN_FLAGBIT;
    }
    for (u32 dir, i=0; KNIGHT_MVMT_CT>i; ++i) {
      dir = dirs[i];
      if (!(HOR_MASK&dir) || !(VER_MASK&dir))
        dirs[i] = INVALID_MVMT_FLAGBIT;
      else
        dirs[i] |= KNIGHT_MVMT_FLAGBIT;
    }
  } else {
    BOOL invalid_piece_value = FALSE;
    switch ((ChessPiece_e)(PIECE_IDX_MASK&piece_type)) {
    case ROOK_IDX:
      dir_count = ROOK_MVMT_CT;
      if (FILE_A < start_loc.coord.x)
        dirs[0] = LEFT_FLAGBIT;
      if (FILE_H > start_loc.coord.x)
        dirs[2] = RIGHT_FLAGBIT;
      if (ROW_8 < start_loc.coord.y)
        dirs[1] = UP_FLAGBIT;
      if (ROW_1 > start_loc.coord.y)
        dirs[3] = DOWN_FLAGBIT;
      for (u32 i=0; ROOK_MVMT_CT>i; ++i)
        if (!dirs[i])
          dirs[i] = INVALID_MVMT_FLAGBIT;
      break;
    case BISHOP_IDX:
      dir_count = BISHOP_MVMT_CT;
      if (FILE_A < start_loc.coord.x) {
        dirs[0] = dirs[1] = LEFT_FLAGBIT;
      }
      if (FILE_H > start_loc.coord.x) {
        dirs[2] = dirs[3] = RIGHT_FLAGBIT;
      }
      if (ROW_8 < start_loc.coord.y) {
        dirs[0] |= UP_FLAGBIT, dirs[2] |= UP_FLAGBIT;
      }
      if (ROW_1 > start_loc.coord.y) {
        dirs[1] |= DOWN_FLAGBIT, dirs[3] |= DOWN_FLAGBIT;
      }
      for (u32 dir, i=0; BISHOP_MVMT_CT>i; ++i) {
        dir = dirs[i];
        if (!(HOR_MASK&dir) || !(VER_MASK&dir))
          dirs[i] = INVALID_MVMT_FLAGBIT;
        else
          dirs[i] |= DIAGONAL_MVMT_FLAGBIT;
      }
      break;
    case KING_IDX:
      iterator->gp_vals.king_vals = (struct s_king_special_use) {
        .castle_moves_tried
            = WHITE_FLAGBIT&piece_type
                ? WHITE_CASTLE_RIGHTS_MASK
                : BLACK_CASTLE_RIGHTS_MASK,
        .cur_castle_move = WHITE_FLAGBIT&piece_type?WK:BK,
        .cur_dir_idx = 0
      };
    case QUEEN_IDX:
      if (FILE_A < start_loc.coord.x) {
        dirs[2]=dirs[3]=dirs[4]=LEFT_FLAGBIT;
      } else {

        // Else preemptively set dirs[<queen/king's left dir idx>] to invalid
        dirs[3]=INVALID_MVMT_FLAGBIT;
      }
      if (FILE_H > start_loc.coord.x) {
        dirs[5]=dirs[6]=dirs[7]=RIGHT_FLAGBIT;
      } else {
        // Else preemptively set dirs[<queen/king's right dir idx>] to invalid
        dirs[6]=INVALID_MVMT_FLAGBIT;
      }
      if (ROW_8 < start_loc.coord.y) {
        dirs[2] |= UP_FLAGBIT,dirs[5] |= UP_FLAGBIT;
        dirs[0] = UP_FLAGBIT;
      } else {
        // Else preemptively set dirs[<queen/king's up dir idx>] to invalid
        dirs[0]=INVALID_MVMT_FLAGBIT;
      }
      if (ROW_1 > start_loc.coord.y) {
        dirs[4] |= DOWN_FLAGBIT, dirs[7] |= DOWN_FLAGBIT;
        dirs[1] = DOWN_FLAGBIT;
      } else {
        // Else preemptively set dirs[<queen/king's down dir idx>] to invalid
        dirs[1]=INVALID_MVMT_FLAGBIT;
      }
      // Since vert-only/horz-only moves set either to appropriate dir flag
      // OR to invalid, we only need to loop thru diagonals
      // Gotta check that both vert and horizontal flagbits are set for the
      // dir indices that rep diagonals. Diagonals are arranged accordingly:
      // 2,4 5,7 for LEFT+UP,LEFT+DOWN , RIGHT+UP,RIGHT+DOWN, respectively.
      // to loop thru, we iterate for i in {0,3} & nested within this for loop,
      // we iterate for j in {2,4}. So we for loop thru left,right respectively
      // and nested loop thru up, down respectively, and combine i+j to form
      // idx offset, (i.e.: dirs[i+j]). This allows us to iterate thru 
      // LEFT+UP,LEFT+DOWN,RIGHT+UP,RIGHT+DOWN in that exact order.
      for (u32 dir, j, i = 0; 6 > i; i+=3) {
        for (j=2; 6 > j; j+=2) {
          dir = dirs[j+i];
          if (!(HOR_MASK&dir) || !(VER_MASK&dir))
            dirs[j+i] = INVALID_MVMT_FLAGBIT;
          else
            dirs[j+i] |= DIAGONAL_MVMT_FLAGBIT;
        }
      }
      dir_count = KING_IDX==(PIECE_IDX_MASK&piece_type)
                        ? KING_MVMT_CT
                        : QUEEN_MVMT_CT;
      break;
    default: invalid_piece_value = TRUE; break;
    }
    assert(!invalid_piece_value);
  }
  assert(dir_count);
  invalid_directions_init_state = FALSE;
  for (u32 i = 0; dir_count > i; ++i) {
    if (dirs[i])
      continue;

    invalid_directions_init_state = TRUE;
    break;
  }
  assert(!invalid_directions_init_state);
  return iterator;
}



BOOL InternalMoveIterator_GetContinuous(ChessBoard_Idx_t *query_in_out_idx,
                                        Mvmt_Dir_e dir) {
  ChessBoard_Idx_t mv = *query_in_out_idx;
  BOOL has_hor, has_ver;
  if (DIAGONAL_MVMT_FLAGBIT&dir) {
    assert((HOR_MASK&dir) && (VER_MASK&dir));
    has_hor = has_ver = TRUE;
  } else {
    has_hor = 0!=(HOR_MASK&dir);
    has_ver = 0!=(VER_MASK&dir);
  }
  if (has_ver) {
    if (UP_FLAGBIT&dir) {
      if (ROW_8 == mv.coord.y)
        return FALSE;
      --mv.coord.y;
    } else {
      if (ROW_1 == mv.coord.y)
        return FALSE;
      ++mv.coord.y;
    }
  }
  if (has_hor) {
    if (LEFT_FLAGBIT&dir) {
      if (FILE_A==mv.coord.x)
        return FALSE;
      --mv.coord.x;
    } else {
      if (FILE_H==mv.coord.x)
        return FALSE;
      ++mv.coord.x;
    }
  }
  *query_in_out_idx = mv;
  return TRUE;
}


BOOL ChessMove_ContinuousMoveDirIterator_HasNext(
                                            InternalMoveIterator_t *iterator) {
  ChessBoard_Idx_t dst = iterator->curmove;
  int dir_ct, iterator_cur_idx = iterator->gp_vals.cur_dir_idx;
  ChessPiece_e piece_type = (ChessPiece_e)(PIECE_IDX_MASK&iterator->piece);
  switch (piece_type) {
    case QUEEN_IDX:
      dir_ct = QUEEN_MVMT_CT;
      break;
    case BISHOP_IDX:
    case ROOK_IDX:
      dir_ct = BISHOP_OR_ROOK_MVMT_COUNT;
      break;
    default:
      dir_ct = 0;
      break;  
  }
  assert(0!=dir_ct);
  if ((const int)dir_ct==iterator_cur_idx)
    return FALSE;
  assert(dir_ct > iterator_cur_idx && 0<=iterator_cur_idx);
  if (InternalMoveIterator_GetContinuous(
                                      &dst, 
                                      iterator->directions[iterator_cur_idx]))
    return TRUE;
  iterator->curmove = iterator->base;
  for (Mvmt_Dir_e *dir = iterator->directions; dir_ct > iterator_cur_idx; 
       ++iterator_cur_idx) {
    if (INVALID_MVMT_FLAGBIT==dir[iterator_cur_idx])
      continue;
    iterator->gp_vals.cur_dir_idx = iterator_cur_idx;
    return TRUE;
  }
  iterator->gp_vals.cur_dir_idx = dir_ct;
  return FALSE;
}

BOOL InternalMoveIterator_IsContinuousMovementIterator(
                                      const InternalMoveIterator_t *iterator) {
  BOOL ret, valid = TRUE;
  switch ((PIECE_IDX_MASK&iterator->piece)) {
  case QUEEN_IDX:
  case ROOK_IDX:
  case BISHOP_IDX:
    ret = TRUE;
    break;
  case PAWN_IDX:
  case KING_IDX:
  case KNIGHT_IDX:
    ret = FALSE;
    break;
  default:
    valid = FALSE;
    break;
  }
  assert(valid);
  return ret;
}

BOOL InternalMoveIterator_ContinuousForceNextDirection(
                                            InternalMoveIterator_t *iterator) {
  int idx;
  if (!InternalMoveIterator_IsContinuousMovementIterator(iterator))
    return FALSE;
  idx = iterator->gp_vals.cur_dir_idx;
  if (InternalMoveIterator_GetDirectionCount(iterator)==idx)
    return TRUE;
  ++idx;
  iterator->gp_vals.cur_dir_idx = idx;
  return TRUE;
}

BOOL ChessMove_KingMoveDirIterator_HasNext(
                                          InternalMoveIterator_t *iterator) {
  const u32 CASTLE_FLAGS = iterator->gp_vals.king_vals.castle_moves_tried;
  int cur_dir_idx;
  BOOL castle_right_found=FALSE;
  if (ChessMove_DiscreteMoveDirIterator_HasNext(iterator))
    return TRUE;
  cur_dir_idx = iterator->gp_vals.cur_dir_idx;
  assert(KING_MVMT_CT==cur_dir_idx);
  if (0==CASTLE_FLAGS)
    return FALSE;
  for (u32 i=1; (ALL_CASTLE_RIGHTS_MASK&i);i<<=1) {
    if (CASTLE_FLAGS&i) {
      iterator->gp_vals.king_vals.cur_castle_move = i;
      castle_right_found = TRUE;
      break;
    }
  }
  assert(castle_right_found);
  return TRUE;
}


BOOL ChessMove_DiscreteMoveDirIterator_HasNext(
                                            InternalMoveIterator_t *iterator) {
  Mvmt_Dir_e *dirs = iterator->directions;
  int dir_ct = 0, iterator_cur_idx = iterator->gp_vals.cur_dir_idx;
  ChessPiece_e piece_type = (ChessPiece_e)(PIECE_IDX_MASK&iterator->piece);
  switch (piece_type) {
  case PAWN_IDX:
    dir_ct = PAWN_MVMT_CT;
    break;
  case KNIGHT_IDX:
    dir_ct = KNIGHT_MVMT_CT;
    break;
  case KING_IDX:
    dir_ct = KING_MVMT_CT;
    break;
  default:
    break;
  }
  assert(0!=dir_ct);
  if ((const int)dir_ct == iterator_cur_idx)
    return FALSE;

  assert(dir_ct > iterator_cur_idx && 0<=iterator_cur_idx);
  for (int i = iterator_cur_idx; dir_ct>i; ++i) {
    if (INVALID_MVMT_FLAGBIT==dirs[i])
      continue;
    if (1==i && PAWN_IDX==piece_type) {
      int promotype = iterator->gp_vals.pawn_vals.cur_promo_type;
      assert(PAWN_PROMOTION_TYPE_CT>=promotype && 0<=promotype);
      if (PAWN_PROMOTION_TYPE_CT==promotype) {
        continue;
      }
    }
    iterator->gp_vals.cur_dir_idx = i;
    return TRUE;
  }
  iterator->gp_vals.cur_dir_idx = dir_ct;
  return FALSE;
}

BOOL InternalMoveIterator_HasNext(InternalMoveIterator_t *iterator) {
  BOOL valid_piece_type = TRUE;
  switch ((ChessPiece_e)(PIECE_IDX_MASK&iterator->piece)) {
  case PAWN_IDX:
  case KNIGHT_IDX:
    return ChessMove_DiscreteMoveDirIterator_HasNext(iterator);
    break;
  case KING_IDX:
    return ChessMove_KingMoveDirIterator_HasNext(iterator);
    break;
  case BISHOP_IDX:
  case ROOK_IDX:
  case QUEEN_IDX:
    return ChessMove_ContinuousMoveDirIterator_HasNext(iterator);
    break;
  default: 
    valid_piece_type = FALSE;
    break;
  }
  assert(valid_piece_type);
  return FALSE;
}

void InternalMoveIterator_ApplyPawnMove(InternalMoveIterator_t *iterator, 
                                        ChessMoveIteration_t *dest) {
  ChessBoard_Idx_t dst = iterator->base;
  Mvmt_Dir_e dir;
  int idx = iterator->gp_vals.cur_dir_idx;
  dir = iterator->directions[idx];
  if (1==idx || 2==idx) {
    if (2==idx) {
      dest->special_flags = MOVE_PAWN_TWO_SQUARE;
    }
    int promo_type = iterator->gp_vals.pawn_vals.cur_promo_type;
    assert(PAWN_PROMOTION_TYPE_CT>=promo_type && 0<=promo_type);
    BOOL promo;
    assert((VER_MASK&dir)==dir && 0!=(VER_MASK&dir));
    if (UP_FLAGBIT==dir) {
      dst.coord.y-=idx;
      promo = 1==idx && ROW_8==dst.coord.y;
    } else {
      dst.coord.y+=idx;
      promo = 1==idx && ROW_1==dst.coord.y;
    }
    if (promo) {
      dest->promotion_flag = PROMOTION_SEL[promo_type];
      dest->dst = dst;
      ++promo_type;
      iterator->gp_vals.pawn_vals.cur_promo_type = promo_type;
      if (PAWN_PROMOTION_TYPE_CT==promo_type)
        ++iterator->gp_vals.cur_dir_idx;
      return;
    }
  } else {
    if (UP_FLAGBIT&dir) {
      --dst.coord.y;
    } else {
      ++dst.coord.y;
    }
    if (LEFT_FLAGBIT&dir) {
      --dst.coord.x;
    } else {
      ++dst.coord.x;
    }
  }
  dest->dst = dst;
  dest->promotion_flag = 0;
  ++iterator->gp_vals.cur_dir_idx;
}



void InternalMoveIterator_ApplyKingMove(InternalMoveIterator_t *iterator,
                                        ChessMoveIteration_t *dest) {
  ChessBoard_Idx_t dst = iterator->base;
  int idx = iterator->gp_vals.cur_dir_idx;

  Mvmt_Dir_e dir = iterator->directions[idx];
  BOOL has_ver, has_hor;

  if (KING_MVMT_CT==idx) {
    u32 castle_mvs = iterator->gp_vals.king_vals.castle_moves_tried,
        cur_castle_mv = iterator->gp_vals.king_vals.cur_castle_move;

    Move_Validation_Flag_e castle_type = MOVE_UNSUCCESSFUL;
    castle_mvs ^= cur_castle_mv;
    iterator->gp_vals.king_vals.castle_moves_tried = castle_mvs;
    switch (cur_castle_mv) {
    case WK:
    case BK:
      castle_type = MOVE_CASTLE_KINGSIDE;
      break;
    case WQ:
    case BQ:
      castle_type = MOVE_CASTLE_QUEENSIDE;
      break;
    }
    assert(MOVE_UNSUCCESSFUL!=castle_type);
    if (MOVE_CASTLE_KINGSIDE==castle_type) {
      dst.coord.x = FILE_G;
    } else {
      dst.coord.x = FILE_C;
    }
    dest->dst = dst;
    dest->special_flags = castle_type;
    return;
  }
  if (DIAGONAL_MVMT_FLAGBIT&dir) {
    assert(0!=(HOR_MASK&dir) && 0!=(VER_MASK&dir)); 
    has_hor=has_ver=TRUE;
  } else {
    has_hor = 0!=(HOR_MASK&dir);
    has_ver = 0!=(VER_MASK&dir);
    assert(has_hor^has_ver);
  }
  if (has_hor) {
    if (LEFT_FLAGBIT&dir) {
      --dst.coord.x;
    } else {
      ++dst.coord.x;
    }
  }
  if (has_ver) {
    if (UP_FLAGBIT) {
      --dst.coord.y;
    } else {
      ++dst.coord.y;
    }
  }
  dest->dst = dst;
  ++idx;
  iterator->gp_vals.cur_dir_idx = idx;
}

void InternalMoveIterator_ApplyKnightMove(InternalMoveIterator_t *iterator,
                                          ChessMoveIteration_t *dest) {
  ChessBoard_Idx_t dst = iterator->base;
  u32 dy, dx;
  int idx = iterator->gp_vals.cur_dir_idx;
  Mvmt_Dir_e dir = iterator->directions[idx];
  if (MVMT_WIDE_FLAGBIT&dir) {
    dx = 2, dy = 1;
  } else {
    dx = 1, dy = 2;
  }
  assert(KNIGHT_MVMT_FLAGBIT&dir);
  assert(0!=(HOR_MASK&dir) && 0!=(VER_MASK&dir));
  if (UP_FLAGBIT&dir) {
    dst.coord.y-=dy;
  } else {
    dst.coord.y+=dy;
  }
  if (LEFT_FLAGBIT&dir) {
    dst.coord.x-=dx;
  } else {
    dst.coord.x+=dx;
  }
  dest->dst = dst;
  ++idx;
  iterator->gp_vals.cur_dir_idx = idx;
}


void InternalMoveIterator_ApplyContinuousMove(InternalMoveIterator_t *iterator,
                                              ChessMoveIteration_t *dest) {
  ChessBoard_Idx_t dst = iterator->curmove;
  int idx = iterator->gp_vals.cur_dir_idx;
  Mvmt_Dir_e dir = iterator->directions[idx];
  
  assert(InternalMoveIterator_GetContinuous(&dst, dir));
  assert((const u64)dst.raw != iterator->curmove.raw);
  iterator->curmove = dest->dst = dst;
}

BOOL InternalMoveIterator_Next(InternalMoveIterator_t *iterator,
                            ChessMoveIteration_t *dest) {
  ChessPiece_e piece_type = PIECE_IDX_MASK&iterator->piece;
  BOOL valid_iterator = TRUE;
  static_assert(0==(sizeof(ChessMoveIteration_t)%sizeof(WORD)));
  Fast_Memset32(dest, 0, sizeof(ChessMoveIteration_t)/sizeof(WORD));

  if (!InternalMoveIterator_HasNext(iterator))
    return FALSE;
  Fast_Memset32(dest, 0, sizeof(ChessMoveIteration_t)/sizeof(WORD));
  switch (piece_type) {
  case PAWN_IDX:
    InternalMoveIterator_ApplyPawnMove(iterator, dest);
    break;
  case KING_IDX:
    InternalMoveIterator_ApplyKingMove(iterator, dest);
    break;
  case KNIGHT_IDX:
    InternalMoveIterator_ApplyKnightMove(iterator, dest);
    break;
  case QUEEN_IDX:
  case ROOK_IDX:
  case BISHOP_IDX:
    InternalMoveIterator_ApplyContinuousMove(iterator, dest);
    break;
  default:
    valid_iterator = FALSE;
    break;
  }
  assert(valid_iterator);
  return TRUE;
}
static_assert(PAWN_MVMT_CT==BISHOP_MVMT_CT && BISHOP_MVMT_CT==ROOK_MVMT_CT);
#define PAWN_OR_BISHOP_OR_ROOK_MVMT_COUNT 4

static_assert(KING_MVMT_CT==QUEEN_MVMT_CT && QUEEN_MVMT_CT==KNIGHT_MVMT_CT);
#define QUEEN_OR_KNIGHT_OR_KING_MVMT_COUNT 8

int InternalMoveIterator_GetDirectionCount(
                                      const InternalMoveIterator_t *iterator) {
  switch ((ChessPiece_e)(PIECE_IDX_MASK&iterator->piece)) {
  case PAWN_IDX:
  case BISHOP_IDX:
  case ROOK_IDX:
    return PAWN_OR_BISHOP_OR_ROOK_MVMT_COUNT;
  case KNIGHT_IDX:
  case QUEEN_IDX:
  case KING_IDX:
    return QUEEN_OR_KNIGHT_OR_KING_MVMT_COUNT;
  default:
    assert(EMPTY_IDX!=iterator->piece);
    assert(NULL == "Data fucked");
    break;
  }
}
