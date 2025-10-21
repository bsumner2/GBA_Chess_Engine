/** (C) 10 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include <GBAdev_functions.h>
#include "chess_board_state_analysis.h"
#include "chess_move_iterator.h"
#include "chess_ai.h"
#include "chess_board.h"
#include "chess_transposition_table.h"
#include "chess_board_state.h"

i16 BoardState_Eval(const BoardState_t *state, Move_Validation_Flag_e last_move);

struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};

static EWRAM_BSS TranspositionTable_t g_ttable;


INLN IWRAM_CODE i16 Piece_Eval(ChessPiece_e piece);
INLN IWRAM_CODE i16 Positional_Eval(ChessPiece_e piece,
                                    ChessBoard_Idx_Compact_t loc);


static IWRAM_CODE ChessAI_MoveSearch_Result_t ChessAI_ABSearch(
                                                      ChessAI_Params_t *params,
                                                      i16 alpha,
                                                      i16 beta);


IWRAM_CODE void ChessAI_Params_Init(ChessAI_Params_t *obj,
                                    BoardState_t *root_state,
                                    int depth,
                                    u32 team) {
  static_assert(0==(sizeof(g_ttable)%sizeof(WORD)));
  Fast_Memset32((obj->ttable = &g_ttable), 
                0UL,
                sizeof(g_ttable)/sizeof(WORD));
  obj->root_state = root_state;
  obj->depth = depth;
  obj->gen = 0;
  obj->team = CONVERT_CTX_MOVE_FLAG(team);
}

IWRAM_CODE void ChessAI_Move(ChessAI_Params_t *ai_params,
                             ChessAI_MoveSearch_Result_t *returned_move) {
  const u32 ini_depth = ai_params->depth;
  *returned_move
        = ChessAI_ABSearch(ai_params,
                           INT16_MIN,
                           INT16_MAX);
  ai_params->depth = ini_depth;
}



ChessAI_MoveSearch_Result_t ChessAI_ABSearch(ChessAI_Params_t *params,
                                             i16 alpha,
                                             i16 beta) {

    // 1. Check transposition table
  TTableEnt_t tt_entry = {
    .key = params->root_state->zobrist,
    .depth = params->depth,
    .gen = params->gen,
  };
  if (TTable_Probe(params->ttable, &tt_entry)) {
    if (tt_entry.depth >= params->depth) {
      return tt_entry.best_move;
    }
  }

  // 2. Base case
  if (params->depth == 0) {
    return (ChessAI_MoveSearch_Result_t) {
      .score = BoardState_Eval(params->root_state, params->last_move),
      .start = INVALID_IDX_COMPACT,
      .dst = INVALID_IDX_COMPACT,
      .mv_flags = 0,
      .promo = 0,
    };
  }

  // 3. Generate moves
  BoardState_t move_applied_state;
  ChessMoveIteration_t move;
  PieceState_Graph_Vertex_t v;
  ChessMoveIterator_t movegen = {0};
  BoardState_t *const PREMOVE_ROOT_STATE = params->root_state;
  const u32 TEAM_PIECE_IDXS_OFS
                = PREMOVE_ROOT_STATE->state.side_to_move&WHITE_TO_MOVE_FLAGBIT
                              ?PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT
                              :0,
            ALLIED_KING = TEAM_PIECE_IDXS_OFS|KING,
            OPP_IDX_OFS = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT^TEAM_PIECE_IDXS_OFS;
  Move_Validation_Flag_e last_move = params->last_move;
  i16 best_move = IS_MAXIMIZING(PREMOVE_ROOT_STATE->state.side_to_move)
                        ? INT16_MIN
                        : INT16_MAX;
  ChessBoard_Idx_Compact_t src;
  BOOL prune=FALSE;
  params->root_state = &move_applied_state;  // switch out params board state 
                                             // ptr to the addr of the mutable 
                                             // local board state.
  for (u32 i,i_base = 0; CHESS_TEAM_PIECE_COUNT>i_base; ++i_base) {
    i = i_base|TEAM_PIECE_IDXS_OFS;
    if (!CHESS_ROSTER_PIECE_ALIVE(PREMOVE_ROOT_STATE->graph.roster, i))
      continue;

    v = PREMOVE_ROOT_STATE->graph.vertices[i];

    src = v.location;
    ChessMoveIterator_Alloc(&movegen, BOARD_IDX_CONVERT(src, NORMAL_IDX_TYPE),
                            PREMOVE_ROOT_STATE,
                            MV_ITER_MOVESET_ALL_MINUS_ALLIED_COLLISIONS
                               |MV_ITER_MOVESET_ORDERED_FLAGBIT);
    while (ChessMoveIterator_HasNext(&movegen)) {
      // copy immutable initial root state to the local mutable root state, 
      // so that it's ready to have the move applied to it.
      Fast_Memcpy32(&move_applied_state,
                    PREMOVE_ROOT_STATE,
                    sizeof(BoardState_t)/sizeof(WORD));
      ChessMoveIterator_Next(&movegen, &move);
      // 4. Apply move
      BoardState_ApplyMove(&move_applied_state,
          (ChessBoard_Idx_Compact_t[2]) {
            src,
            BOARD_IDX_CONVERT(move.dst, COMPACT_IDX_TYPE)
          }, 
          move.special_flags);
      if (BoardState_KingInCheck(&move_applied_state,
                                 ALLIED_KING,
                                 OPP_IDX_OFS))
        continue;

      // recursed, so now all we need to do 
      --params->depth;
      params->last_move = move.special_flags;
      {
        ChessAI_MoveSearch_Result_t mv = ChessAI_ABSearch(params,
                                              alpha,
                                              beta);
       
        // 7. Alpha-beta logic
        if (IS_MAXIMIZING(PREMOVE_ROOT_STATE->state.side_to_move)) {
          if (mv.score > best_move) {
            best_move = mv.score;
            tt_entry.best_move = (ChessAI_MoveSearch_Result_t){
              .dst = BOARD_IDX_CONVERT(move.dst, COMPACT_IDX_TYPE),
              .promo = move.promotion_flag,
              .mv_flags = move.special_flags,
              .score = mv.score,
              .start = src
            };  /* update ttable entry that will be 
                                       * tabulated upon exit. */
            if (mv.score > alpha)
              alpha = mv.score;
          }
          if (mv.score >= beta) {
            prune = TRUE;
            break;
          }
        } else {
          if (mv.score < best_move) {
            best_move=mv.score;
            tt_entry.best_move = (ChessAI_MoveSearch_Result_t){
              .dst = BOARD_IDX_CONVERT(move.dst, COMPACT_IDX_TYPE),
              .promo = move.promotion_flag,
              .mv_flags = move.special_flags,
              .score = mv.score,
              .start = src
            };  /* update ttable entry that will be 
                                       * tabulated upon exit. */
            if (mv.score < beta)
              beta=mv.score;
          }
          if (mv.score <= alpha) {
            prune = TRUE;
            break;
          }
        }
      }
      // 6. Reset params->depth to this call's param values.
      // Don't need to reset last_move
      ++params->depth;
      if (beta <= alpha) break; // prune
    }
    ChessMoveIterator_Dealloc(&movegen);
    if (prune) {
      break;
    }
  }
  // Restore pointer to original immutable copy of board_state
  params->root_state = PREMOVE_ROOT_STATE;
  params->last_move = last_move;

  // 8. Store in TT
  tt_entry.key = params->root_state->zobrist;
  tt_entry.gen = params->gen;
  tt_entry.depth = params->depth;

  TTable_Insert(params->ttable, &tt_entry);

  return tt_entry.best_move;
}

IWRAM_CODE i16 Piece_Eval(ChessPiece_e piece) {
  switch (piece) {
  case PAWN_IDX:
    return 100;
  case BISHOP_IDX:
  case KNIGHT_IDX:
    return 300;
  case ROOK_IDX:
    return 500;
  case QUEEN_IDX:
    return 900;
  case KING_IDX:
  default:
    return 0;
    break;
  }
}

IWRAM_CODE i16 Positional_Eval(ChessPiece_e piece,
                               ChessBoard_Idx_Compact_t loc) {
  ChessBoard_File_e x=loc.coord.x;
  ChessBoard_Row_e y=loc.coord.y;
  if (PAWN_IDX==piece)
    return 0;
  if (x<FILE_C || x>FILE_F)
    return 0;
  if (y<ROW_6 || y>ROW_3)
    return 0;
  return 15;
}

IWRAM_CODE i16 BoardState_Eval(const BoardState_t *state, Move_Validation_Flag_e last_move) {

  PieceState_Graph_Vertex_t v;
  const PieceState_Graph_Vertex_t *vertices = state->graph.vertices;
  const u32 TEAM_OFS = (state->state.side_to_move&WHITE_TO_MOVE_FLAGBIT)
                              ? PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT
                              : 0,
            OPP_OFS = TEAM_OFS^PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT,
            KING_ID = KING|TEAM_OFS;
  const ChessPiece_Roster_t ROSTER_STATE = state->graph.roster;
  const ChessPiece_e (*board)[CHESS_BOARD_ROW_COUNT] = state->board;
  const u16 STATE_CASTLE_FLAGS=state->state.castle_rights;
  i16 score=0, base, tactical;
  ChessPiece_e piece;
  BOOL is_white, is_negative;
  for (u32 i=0; CHESS_TOTAL_PIECE_COUNT>i; ++i) {
    if (!CHESS_ROSTER_PIECE_ALIVE(ROSTER_STATE, i))
      continue;
    v=vertices[i];
    piece = board[BOARD_IDX(v.location)];
    is_white = WHITE_FLAGBIT==(piece&PIECE_TEAM_MASK);
    base = Piece_Eval(PIECE_IDX_MASK&piece);
    tactical=0;
    tactical+=10*v.attacking_count;
    tactical+=5*v.defending_count;
    tactical+=Positional_Eval(PIECE_IDX_MASK&piece, v.location);
    score += is_white ? (base+tactical)
                      : -(base+tactical);
  }
  for (u16 flag=1; flag&ALL_CASTLE_RIGHTS_MASK; flag<<=1) {
    /*  Truth table:
     *  curflag HIGH | white flag | positive score change
     *       0       |   0        |    1        // black lost castle right (+)
     *       0       |   1        |    0        // white lost castle right (-)
     *       1       |   0        |    0        // black has castle right  (-)
     *       1       |   1        |    1        // white has castle right  (+)
     *       So if we do:
     *       (0!=(STATE_CASTLE_FLAGS&flag)) ^ (0!=(flag&WHITE_CASTLE_MASK))
     *       which is <has said castle right> XOR <flag is a white castle right>
     *       we get inverse of positive score change (so negative score change):
    */
    if ((0!=(STATE_CASTLE_FLAGS&flag))^(0!=(WHITE_CASTLE_RIGHTS_MASK&flag)))
      score -= 25;
    else
      score += 25;
  }
  if (last_move&MOVE_CASTLE_MOVE_FLAGS_MASK) {
    score+=(WHITE_TO_MOVE_FLAGBIT==state->state.side_to_move) ? 50 : -50;
  }
  return score;

}
