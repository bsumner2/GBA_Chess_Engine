/** (C) 16 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#define __TRANSPARENT_BOARD_STATE__

#include <GBAdev_functions.h>
#include <GBAdev_memmap.h>
#include <GBAdev_types.h>
#include <assert.h>
#include "chess_board.h"
#include "chess_move_iterator.h"
#include "chess_move_iterator_block_allocator.h"
#include "debug_io.h"

struct s_chess_move_iterator_private {
  u32 cur_move;
  ChessMoveIteration_t *moves;
};

#define INVALID_IDX_MASK 0xFFFFFFF8FFFFFFF8ULL

// Queens which have the most total possible movement directions, have, at most,
// 27 moves when placed at most central squares of board. Therefore, we can
// expect count to never exceed 27
#define MAX_MOVE_CANDIDATES 27

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
extern EWRAM_CODE InternalMoveIterator_t *InternalMoveIterator_Init(
                                    InternalMoveIterator_t *iterator,
                                    ChessBoard_Idx_t start_loc,
                                    ChessPiece_e piece_type);
extern EWRAM_CODE void InternalMoveIterator_Uninit(InternalMoveIterator_t *iterator);

extern EWRAM_CODE BOOL InternalMoveIterator_HasNext(InternalMoveIterator_t *iterator);
extern EWRAM_CODE BOOL InternalMoveIterator_Next(InternalMoveIterator_t *iterator,
                               ChessMoveIteration_t *dest);
extern EWRAM_CODE BOOL InternalMoveIterator_IsContinuousMovementIterator(
                                const InternalMoveIterator_t *iterator);
extern EWRAM_CODE BOOL InternalMoveIterator_ContinuousForceNextDirection(
                                              InternalMoveIterator_t *iterator);

static IWRAM_BSS ChessMoveIteration_t _L_move_buffer[MAX_MOVE_CANDIDATES];






// We can do this thanks to GBA being single-threaded.
static const EWRAM_BSS BoardState_t *_L_cur_board_state=NULL;
static ChessBoard_Idx_t _L_cur_piece_location = INVALID_IDX;



static EWRAM_CODE int Capture_Eval(ChessBoard_Idx_t loc);
static EWRAM_CODE int Knight_Move_Eval(ChessBoard_Idx_t loc);
static EWRAM_CODE int Promo_Flag_Eval(int promo_flag);
static EWRAM_CODE int __MoveIterationCmp(const void *a, const void *b);

EWRAM_CODE int Capture_Eval(ChessBoard_Idx_t loc) {
  PieceState_Graph_Vertex_t capt_graphnode;
  capt_graphnode 
    = _L_cur_board_state
              ->graph.vertices[_L_cur_board_state->graph.vertex_hashmap[
                                                              BOARD_IDX(loc)]];
  return 2*capt_graphnode.attacking_count 
                - capt_graphnode.defending_count;
}

EWRAM_CODE int Knight_Move_Eval(ChessBoard_Idx_t loc) {
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

EWRAM_CODE int Promo_Flag_Eval(int promo_flag) {
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


EWRAM_CODE int __MoveIterationCmp(const void *a, const void *b) {
  const ChessMoveIteration_t *lhs=a, *rhs=b;
  assert(_L_cur_board_state!=NULL);
  ChessPiece_e start_piece, lhs_dst_piece, rhs_dst_piece;
  BOOL lhs_empty;
  lhs_dst_piece = _L_cur_board_state->board[BOARD_IDX(lhs->dst)];
  rhs_dst_piece = _L_cur_board_state->board[BOARD_IDX(rhs->dst)];

  assert(INVALID_IDX_RAW_VAL != _L_cur_piece_location.raw);
  start_piece = PIECE_IDX_MASK
                  & _L_cur_board_state->board[BOARD_IDX(_L_cur_piece_location)];

  lhs_empty = (EMPTY_IDX==lhs_dst_piece);
  if (lhs_empty^(EMPTY_IDX==rhs_dst_piece)) {  // 1 operand empty, other not
    return lhs_empty ? 1 : -1;                 // and if lhs is the empty 1,
                                               // positive cmp return. This
                                               // is done because we want
                                               // better moves at leftmost 
                                               // side (lowermost addr),
                                               // as iterator iterates 
                                               // to right (increment addr)
                                               // as we go thru move buf
  }
  if (lhs_empty) {
    // if both spots equal, just evaluate based on distance for cmp, OR
    // (special cases: castle, promo, and knight moving into edge or corner
    // Special Cases:
    if (KNIGHT_IDX==start_piece) {
      return Knight_Move_Eval(rhs->dst) - Knight_Move_Eval(lhs->dst);
    } else if (KING_IDX==start_piece || PAWN_IDX==start_piece) {
      if (PAWN_IDX==start_piece) {
        if (lhs->promotion_flag != rhs->promotion_flag) {
          // THis works because default value of ZERO means no promo occurred,
          // and then promotion_flag
          return Promo_Flag_Eval(rhs->promotion_flag)
                  - Promo_Flag_Eval(lhs->promotion_flag);
        }
      }
      if (MOVE_SPECIAL_MOVE_FLAGS_MASK&lhs->special_flags) {
        return MOVE_SPECIAL_MOVE_FLAGS_MASK&rhs->special_flags ? 0 : -1;
      } else if (MOVE_SPECIAL_MOVE_FLAGS_MASK&rhs->special_flags) {
        return 1;
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
    return rhs_dist_magnitude - lhs_dist_magnitude;

  }
  return Capture_Eval(rhs->dst) - Capture_Eval(lhs->dst);


  
  
  
}
EWRAM_CODE BOOL ChessMoveIterator_Alloc(ChessMoveIterator_t *dst_iterator,
                             ChessBoard_Idx_t piece_location,
                             const BoardState_t *state,
                             ChessMoveIterator_MoveSetMode_e mode) {

  InternalMoveIterator_t iter;
  ChessMoveIterator_t iterator={0};
  ChessMoveIteration_t cur_mv;
  const ChessPiece_e (*BOARD_DATA)[CHESS_BOARD_ROW_COUNT] = state->board;
  ChessMoveIteration_t *moves;
  const u32 ALLIED_TEAM_FLAGBIT 
                      = BOARD_DATA[BOARD_IDX(piece_location)]&PIECE_TEAM_MASK,
            OPP_TEAM_FLAGBIT
                      = PIECE_TEAM_MASK^ALLIED_TEAM_FLAGBIT;

  ChessPiece_e mv_piece = BOARD_DATA[BOARD_IDX(piece_location)], curpiece;
  u32 count = 0;
  BOOL ordered = MV_ITER_MOVESET_SETTING_ENABLED(mode, ORDERED);
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
#if 1
    assert(0ULL==(cur_mv.dst.raw&INVALID_IDX_MASK));
#else
    if (0ULL!=(cur_mv.dst.raw&INVALID_IDX_MASK)) {
      DebugMsgF(DEBUG_MSG_RETURNS,
                "Invalid board index given: (%u, %u).\n"
                "Interator Info: iterator_type = 0x%04hX\n",
                cur_mv.dst.coord.x, cur_mv.dst.coord.y, iter.piece);
      do IRQ_Sync(1<<IRQ_KEYPAD); while (!KEY_STROKE(A));
      DebugMsgF(DEBUG_MSG_RETURNS,
                "moving piece start loc: \x1b[0x44E4](%d,%d)\x1b[0x1484]"
                "cur_mv = {\n\t.dst= \x1b[0x44E4](%d,%d)\x1b["
                TO_EXP_STR(DEF_ERR_MSG_CLR) "],\n\t"
                ".promotion_flag = \x1b[0x44E4]%d\x1b[" 
                TO_EXP_STR(DEF_ERR_MSG_CLR)"],\n\t"
                "special_flags = \x1b[0x44E4]0x%04hX\x1b[" 
                TO_EXP_STR(DEF_ERR_MSG_CLR)"]\n}",
                piece_location.arithmetic.x,piece_location.arithmetic.y,
                cur_mv.dst.arithmetic.x, cur_mv.dst.arithmetic.y,
                cur_mv.promotion_flag, cur_mv.special_flags);
      do IRQ_Sync(1<<IRQ_KEYPAD); while (!KEY_STROKE(A));
#define FMT_WITH_CLR(conversion_flag)\
      "\x1b[0x44E4]" conversion_flag "\x1b[" TO_EXP_STR(DEF_ERR_MSG_CLR) "]"
      DebugMsgF(DEBUG_MSG_EXITS,
                "internal_iterator.directions = {\n\t"
                FMT_WITH_CLR("0x%04hX") ", " FMT_WITH_CLR("0x%04hX") ", "
                FMT_WITH_CLR("0x%04hX") ", " FMT_WITH_CLR("0x%04hX") ", "
                FMT_WITH_CLR("0x%04hX") ",\n\t" FMT_WITH_CLR("0x%04hX") ", "
                FMT_WITH_CLR("0x%04hX") ", " FMT_WITH_CLR("0x%04hX") "\n}\n"
                "internal_iterator.cur_dir_idx = " FMT_WITH_CLR("%hu"),
                iter.directions[0], iter.directions[1], iter.directions[2],
                iter.directions[3], iter.directions[4], iter.directions[5],
                iter.directions[6], iter.directions[7], 
                iter.gp_vals.cur_dir_idx);
    }
#endif
    curpiece = BOARD_DATA[BOARD_IDX(cur_mv.dst)];
    if (MOVE_SPECIAL_MOVE_FLAGS_MASK&cur_mv.special_flags) {
      if (MOVE_CASTLE_MOVE_FLAGS_MASK&cur_mv.special_flags) {
        ChessBoard_Idx_t check_against;
        u32 flag;
        i32 dx;
        BOOL valid=TRUE;
        assert(KING_IDX==(PIECE_IDX_MASK&mv_piece));
        if (MOVE_CASTLE_KINGSIDE&cur_mv.special_flags) {
          flag = KINGSIDE_SHAMT_INVARIANT;
          check_against.coord.x = FILE_G;
          dx=-1;
        } else {
          flag = QUEENSIDE_SHAMT_INVARIANT;
          check_against.coord.x = FILE_C;
          dx=+1;
        }
        if (WHITE_FLAGBIT&mv_piece) {
          flag<<=CASTLE_RIGHTS_WHITE_FLAGS_SHAMT;
          check_against.coord.y = ROW_1;
        } else {
          flag<<=CASTLE_RIGHTS_BLACK_FLAGS_SHAMT;
          check_against.coord.y = ROW_8;
        }
        if (0==(flag&state->state.castle_rights))
          continue;
        assert((
              (const ChessBoard_Idx_t){
                .coord = {
                  .x=FILE_E,
                  .y=check_against.coord.y
                }
              }).raw==piece_location.raw);
        if ((const u64)check_against.raw != cur_mv.dst.raw) {
          DebugMsgF(DEBUG_MSG_EXITS,
              "Movement type = \x1b[0x44E4]%s\x1b[0x1484]\n"
              "So expected idx (\x1b[0x44E4]FILE_%c\x1b[0x1484], "
              "\x1b[0x44E4]ROW_%d\x1b[0x1484]),\nbut cur_mv.dst = "
              "(\x1b[0x44E4]FILE_%c\x1b[0x1484], "
              "\x1b[0x44E4]ROW_%d\x1b[0x1484])",
              (cur_mv.special_flags&MOVE_CASTLE_QUEENSIDE 
                            ? "MOVE_CASTLE_QUEENSIDE"
                            : "MOVE_CASTLE_KINGSIDE"),
              ('A'+check_against.arithmetic.x), (8-check_against.arithmetic.y),
              ('A'+cur_mv.dst.arithmetic.x), (8-check_against.arithmetic.y));
        }

        if (EMPTY_IDX!=curpiece)
          continue;
        check_against.coord.x = FILE_E > check_against.coord.x 
                                       ? FILE_A
                                       : FILE_H;
        if ((ALLIED_TEAM_FLAGBIT|ROOK_IDX)
                !=BOARD_DATA[BOARD_IDX(check_against)])
          continue;
        check_against.arithmetic.x += dx;
        for (i32 file = check_against.arithmetic.x; FILE_E!=file;
             check_against.arithmetic.x = (file+=dx)) {
          ensure((VALID_FILE_MASK&file)==(u32)file, 
                  "file = \x1b[0x44E4]%ld\x1b[0x1484]\nVALID_FILE_MASK&file = "
                  "\x1b[0x44E4]%lu\x1b[0x1484]", file, file&VALID_FILE_MASK);
          if (EMPTY_IDX!=BOARD_DATA[BOARD_IDX(check_against)]) {
            valid = FALSE;
            break;
          }
        }
        if (!valid)
          continue;
      } else if (MOVE_PAWN_TWO_SQUARE&cur_mv.special_flags) {
        ChessBoard_Idx_t middle;
        assert(PAWN_IDX==(PIECE_IDX_MASK&mv_piece));
        if (EMPTY_IDX!=curpiece)
          continue;
        middle.coord.x = piece_location.coord.x;
        
        if (WHITE_FLAGBIT&mv_piece) {
          assert(ROW_2==piece_location.coord.y && ROW_4==cur_mv.dst.coord.y);
          middle.coord.y = ROW_3;
          
        } else {
          assert(ROW_7==piece_location.coord.y && ROW_5==cur_mv.dst.coord.y);
          middle.coord.y = ROW_6;
        }
        if (EMPTY_IDX!=BOARD_DATA[BOARD_IDX(middle)])
          continue;
        
        

      } else {
        DebugMsgF(DEBUG_MSG_EXITS, "Unexpected move flag yielded by internal "
            "move iterator: \x1b[0x44E4]0x%04hX\x1b[0x1484].\nHowever, the "
            "only special flags set by this iterator are \x1b[0x44E4]0x%04hX"
            "\x1b[0x1484]", cur_mv.special_flags, 
            MOVE_CASTLE_MOVE_FLAGS_MASK|MOVE_PAWN_TWO_SQUARE);
      }
      
      
    } else if (PAWN_IDX==(mv_piece&PIECE_IDX_MASK)) {
      const ChessBoard_Row_e MV_ROW = cur_mv.dst.coord.y;
      const ChessBoard_File_e MV_FILE = cur_mv.dst.coord.x;
      if (MV_FILE == piece_location.coord.x) {
        if (EMPTY_IDX!=curpiece)
          continue;
      } else if (EMPTY_IDX==curpiece) {
        if (MV_FILE==state->state.ep_file) {
          const ChessBoard_Row_e STARTING_ROW = piece_location.coord.y;
          if (WHITE_FLAGBIT==ALLIED_TEAM_FLAGBIT) {
            if (ROW_6!=MV_ROW)  // can't be empty pawn attack and NOT
                                // En Passent, so invalid move; continue
              continue;
            assert(ROW_5==STARTING_ROW);  // if it is proper
                                          // en passent move row for
                                          // white, then start loc
                                          // MUST be ROW_5
          } else {
            assert(BLACK_FLAGBIT==ALLIED_TEAM_FLAGBIT);
            // Mirror EP validity check logic for black
            if (ROW_3!=MV_ROW)  // Same logic here. Continue for invalid move
              continue;
            assert(ROW_4==STARTING_ROW);  // Same logic here too.
          }
          assert((OPP_TEAM_FLAGBIT|PAWN_IDX)==BOARD_DATA[STARTING_ROW][MV_FILE]);
          cur_mv.special_flags |= MOVE_CAPTURE|MOVE_EN_PASSENT;
        } else {
          continue;
        }
      }
    }
    switch (mode) {
    case MV_ITER_MOVESET_ALL_MINUS_ALLIED_COLLISIONS:
      if (EMPTY_IDX==curpiece) {
        _L_move_buffer[count++] = cur_mv;
        continue;
      }
      ensure(curpiece&PIECE_TEAM_MASK, 
          "curpiece = \x1b[0x44E4]0x%04hX\x1b[0x1484]"
          "\n\t(\x1b[0x44E4]%s\x1b[0x1484])\n"
          "curpiece was at ( \x1b[0x44E4]%u\x1b[0x1484], "
          "\x1b[0x44E4]%u\x1b[0x1484] )",
          curpiece, DebugIO_ChessPiece_ToString(curpiece), cur_mv.dst.coord.x,
          cur_mv.dst.coord.y);
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
        if (0==(MOVE_EN_PASSENT&cur_mv.special_flags))
          continue;
        // Otherwise, if cap did occur, and move dest (curpiece) is empty,
        // then en passent it mustve been.
        if ((MOVE_EN_PASSENT|MOVE_CAPTURE)!=cur_mv.special_flags) {
          DebugMsgF(DEBUG_MSG_EXITS,
              "cur_mv.special_flags!=("
              "\x1b[0x44E4]MOVE_EN_PASSENT\x1b[0x1484]|\x1b[0x44E4]MOVE_CAPTURE"
              "\x1b[0x1484])\ncur_mv.special_flags = "
              "\x1b[0x44E4]0x%04hX\x1b[0x1484]",
              cur_mv.special_flags);
        }
        assert(PAWN_IDX==(mv_piece&PIECE_IDX_MASK));
        _L_move_buffer[count++] = cur_mv;
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
      continue;
      break;
    default:
      assert(0);
    }
  }
  if (0==count) {
    iterator.priv = NULL;
    iterator.size = 0;
    return TRUE;
  }
  iterator.priv = MoveIterator_PrivateFields_Allocate();
  assert(NULL!=iterator.priv);
   moves = MoveBufferHeap_Alloc(count);
   assert(NULL!=moves);
  *iterator.priv = (ChessMoveIterator_PrivateFields_t){ 
    .cur_move = 0,
    .moves = moves
  };
  assert(0==iterator.priv->cur_move && NULL!=iterator.priv->moves);
  static_assert(0==(sizeof(ChessMoveIteration_t)%sizeof(WORD)));
  Fast_Memcpy32(moves,
                _L_move_buffer,
                count*sizeof(ChessMoveIteration_t)/sizeof(WORD));
  if (ordered) {
    // Need to pass current state to src file local, _L_cur_board_state
    _L_cur_board_state = state;
    _L_cur_piece_location = piece_location;
    qsort(moves,
          count,
          sizeof(ChessMoveIteration_t),
          __MoveIterationCmp);
  }
  iterator.size = count;
  *dst_iterator = iterator;
  return TRUE;
}

EWRAM_CODE BOOL ChessMoveIterator_HasNext(const ChessMoveIterator_t *iterator) {
  assert(NULL!=iterator);
  if (NULL==iterator->priv) {
    assert(0==iterator->size);
    return FALSE;
  }
  assert(0!=iterator->size && NULL!=iterator->priv->moves);
  return iterator->size > iterator->priv->cur_move;
}

EWRAM_CODE BOOL ChessMoveIterator_Next(ChessMoveIterator_t *iterator,
                            ChessMoveIteration_t *ret_mv) {
  if (!ChessMoveIterator_HasNext(iterator))
    return FALSE;
  if (NULL==ret_mv)
    return FALSE;
  ChessMoveIteration_t *moves = iterator->priv->moves;
  *ret_mv = moves[iterator->priv->cur_move++];
  return TRUE;
}

EWRAM_CODE BOOL ChessMoveIterator_Dealloc(ChessMoveIterator_t *iterator) {
  if (NULL==iterator)
    return FALSE;
  if (NULL==iterator->priv) {
    assert(0==iterator->size);
    return TRUE;
  }
  assert(0!=iterator->size && NULL!=iterator->priv->moves);
  MoveBufferHeap_Dealloc(iterator->priv->moves);
  assert(MoveIterator_PrivateFields_Deallocate(iterator->priv));
  static_assert(sizeof(ChessMoveIterator_t)==sizeof(u64));
  *((u64*)iterator) = 0ULL;
  assert(0==iterator->size && NULL==iterator->priv);
  return TRUE;
}
