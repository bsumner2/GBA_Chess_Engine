/** (C) 10 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include <GBAdev_functions.h>
#include <GBAdev_memmap.h>
#include "GBAdev_memdef.h"
#include "GBAdev_types.h"
#include "GBAdev_util_macros.h"
#include "chess_ai_types.h"
#include "chess_board_state_analysis.h"
#include "chess_gameloop.h"
#include "chess_move_iterator.h"
#include "chess_ai.h"
#include "chess_board.h"
#include "chess_transposition_table.h"
#include "chess_board_state.h"
#include "debug_io.h"

#ifdef _DEBUG_BUILD_
#ifdef _DEBUG_OVERRIDE_KSYNC_
#define Debug_Ksync(unused0, unused1)
#else
#define Debug_Ksync(k, v) Ksync(k, v)
#endif  /* Override stepwise keypad control of DFS traversal */
#define _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_
#else
#define Debug_Ksync(unused1, unused2)
#endif  /* Debug mode DFS traversal stepping control macros */
IWRAM_CODE i16 BoardState_Eval(const BoardState_t *state,
                               Move_Validation_Flag_e last_move);

struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};

typedef union u_chess_ai_piece_tracker {
  struct s_chess_ai_piece_tracker {
    ChessPiece_Roster_t roster;
    Obj_Attr_t *pieces_captured[2][CHESS_TEAM_PIECE_COUNT];
    int capcount[2];
    void *dummy;
  } data;
  ChessPiece_Tracker_t ctx_compat;
} ChessAI_PieceTracker_t;



static EWRAM_BSS TranspositionTable_t g_ttable;
#ifdef _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_
#include "chess_obj_sprites_data.h"
#define SPRITE_VRAM_TILE_IDX(sprite_type) (2*TILES_PER_CSPR*sprite_type)
static EWRAM_BSS ChessObj_Set_t _L_move_sprites;
static EWRAM_BSS ChessAI_PieceTracker_t _L_captured_sprite_tracker;

static IWRAM_CODE void ChessAI_ResetPieceVisualizer(
                                              const BoardState_t *board_state);
static IWRAM_CODE void ChessAI_SearchVisualize_Move(
                                      const BoardState_t *move_applied_state,
                                      const ChessMoveIteration_t *move,
                                      u32 moving_idx);
INLN IWRAM_CODE void UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(int sel_idx,
                                                      ChessBoard_Idx_t coord);

#elifdef _AI_VISUALIZE_MOVE_CANDIDATES_
#include "chess_obj_sprites_data.h"
#define SPRITE_VRAM_TILE_IDX(sprite_type) (2*TILES_PER_CSPR*sprite_type)
#define SEL_INITIALIZER ((Obj_Attr_t) {\
    .attr0.regular = {  \
      .affine_mode=FALSE,  \
      .disable = FALSE,  \
      .mode = OBJ_GFX_MODE_BLEND,  \
      .shape = CHESS_OATTR_SHAPE_VALUE,  \
      .pal_8bpp = TRUE,  \
      .mosaic = FALSE,  \
      .y = 0  \
    },  \
    .attr1.regular = {  \
      .hflip = FALSE,  \
      .obj_size = CHESS_OATTR_SIZE_VALUE,  \
      .vflip = FALSE,  \
      .x = 0,  \
    },  \
    .attr2 = {  \
      .priority = 1,  \
      .sprite_idx = SPRITE_VRAM_TILE_IDX(EMPTY_IDX)  \
    }  \
  })
static EWRAM_BSS ChessObj_Mvmt_Sel_t _L_sels;
INLN IWRAM_CODE void UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(int sel_idx,
                                                      ChessBoard_Idx_t coord);
#else
#define UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(dummy_field0, dummy_field1)
#endif  /* _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_ */

INLN IWRAM_CODE i16 Piece_Eval(ChessPiece_e piece);
INLN IWRAM_CODE i16 Positional_Eval(ChessPiece_e piece,
                                    ChessBoard_Idx_Compact_t loc);


static IWRAM_CODE ChessAI_MoveSearch_Result_t ChessAI_ABSearch(
                                                      ChessAI_Params_t *params,
                                                      i16 alpha,
                                                      i16 beta);
#ifdef _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_


IWRAM_CODE void ChessAI_SearchVisualize_Move(
                                      const BoardState_t *move_applied_state,
                                      const ChessMoveIteration_t *move,
                                      u32 moving_idx) {
  static const ChessBoard_Idx_t CASTLE_ROOK_LOCS[4] = {
    {.coord={.x=FILE_F, .y=ROW_1}}, {.coord={.x=FILE_D, .y=ROW_1}},
    {.coord={.x=FILE_F, .y=ROW_8}}, {.coord={.x=FILE_D, .y=ROW_8}},
  };
  static const u32 CASTLE_ROOK_IDXS[4] = {
    WHITE_ROSTER_ID(ROOK1),
    WHITE_ROSTER_ID(ROOK0), 
    BLACK_ROSTER_ID(ROOK1),
    BLACK_ROSTER_ID(ROOK0)
  };
  u16 castle_flags = MOVE_CASTLE_MOVE_FLAGS_MASK&move->special_flags;
  if (MOVE_CAPTURE&move->special_flags) {
    int capidx_abs, capidx, capteamidx, capcount;
    u32 capdiff = _L_captured_sprite_tracker.data.roster.all
                     ^move_applied_state->graph.roster.all;
    assert(capdiff);
    capidx_abs = __builtin_ctz(capdiff);
    capteamidx = 0!=(capidx_abs&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT);
    assert(0==capteamidx ^ 1==capteamidx);
    capidx =  capidx_abs&PIECE_ROSTER_ID_MASK;
    _L_captured_sprite_tracker.data.roster = move_applied_state->graph.roster;
    capcount = ++_L_captured_sprite_tracker.data.capcount[capteamidx];
    _L_captured_sprite_tracker.data
                .pieces_captured[capteamidx][capcount-1]
                                = &_L_move_sprites.pieces[capteamidx][capidx];
    qsort(&_L_captured_sprite_tracker.ctx_compat.pieces_captured[capteamidx][0],
          capcount,
          sizeof(void*),
          ObjAttrCmp);
    ChessGame_DrawCapturedTeam(&_L_move_sprites.pieces[0][0],
                               &_L_captured_sprite_tracker.ctx_compat,
                               capteamidx?WHITE_FLAGBIT:BLACK_FLAGBIT);
/*    Debug_Ksync(START, KSYNC_DISCRETE);
    ensure(0, "Capture should have been displayed here at "
        "( \x1b[0x44E4]FILE_%c\x1b[0x1484 , \x1b[0x44E4]ROW_%d\x1b[0x1484] )"
        ", but failed to display the capture.\n"
        "\tMoving idx = %d\n\tCaptured idx = %lu\n",
        'A'+move->dst.coord.x, 7&move->dst.coord.y, capidx, moving_idx);*/

  } else if (castle_flags) {
    int rook_idx = castle_flags>>(MOVE_VALIDATION_CASTLE_FLAGS_SHAMT+1);
    rook_idx += (PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT&moving_idx)
                            ? 0
                            : 2;
    assert((rook_idx&3)==rook_idx);
    rook_idx = CASTLE_ROOK_IDXS[rook_idx];
    UPDATE_PIECE_SPRITE_LOCATION(
        ((Obj_Attr_t*)_L_move_sprites.pieces)+CASTLE_ROOK_IDXS[rook_idx],
        CASTLE_ROOK_LOCS[rook_idx]);
    OAM_Copy(&OAM_ATTR[rook_idx], 
             ((Obj_Attr_t*)_L_move_sprites.pieces)+rook_idx, 1);
  }
  UPDATE_PIECE_SPRITE_LOCATION(
      ((Obj_Attr_t*)_L_move_sprites.pieces)+moving_idx,
      move->dst);
  if (move->promotion_flag)
    _L_move_sprites.pieces[moving_idx]->attr2.sprite_idx
              = SPRITE_VRAM_TILE_IDX(move->promotion_flag);
  OAM_Copy(OAM_ATTR,
           (Obj_Attr_t*)_L_move_sprites.pieces,
           CHESS_TOTAL_PIECE_COUNT);
  Debug_Ksync(A, KSYNC_CONTINUOUS);
}

IWRAM_CODE void ChessAI_ResetPieceVisualizer(const BoardState_t *board_state) {
  ChessBoard_Idx_t cur_loc;
  const ChessPiece_e (*BOARD_DATA)[CHESS_BOARD_FILE_COUNT] = board_state->board;
  const PieceState_Graph_Vertex_t *cur_team_verts;
  Obj_Attr_t *cur_team;
  Obj_Attr_t **cur_capteam;
  int *cur_capcount;
  _L_captured_sprite_tracker.data.roster = board_state->graph.roster;
  for (u32 j,i = 0; 2>i; ++i) {
    cur_team = _L_move_sprites.pieces[i];
    cur_capteam = _L_captured_sprite_tracker.data.pieces_captured[i];
    cur_capcount = &_L_captured_sprite_tracker.data.capcount[i];
    *cur_capcount = 0;
    cur_team_verts = &board_state->graph.vertices[i<<4];
    for (j = 0; CHESS_TEAM_PIECE_COUNT>j; ++j) {
      if (!CHESS_ROSTER_PIECE_ALIVE(_L_captured_sprite_tracker.data.roster, 
                                    i<<4|j)) {
        cur_capteam[(*cur_capcount)++] = &cur_team[j];
        continue;
      }
      cur_loc = BOARD_IDX_CONVERT(cur_team_verts[j].location, NORMAL_IDX_TYPE);
      UPDATE_PIECE_SPRITE_LOCATION(cur_team+j, cur_loc);
      if (PAWN0 > j)
        continue;
      cur_team[j].attr2.sprite_idx
        = SPRITE_VRAM_TILE_IDX(BOARD_DATA[BOARD_IDX(cur_loc)])
          + ((!i)<<1)*TILES_PER_CSPR*Chess_sprites_Glyph_Count;
    }

    qsort(cur_capteam, *cur_capcount, sizeof(void*), ObjAttrCmp);
  }
  OAM_Copy(OAM_ATTR, &_L_move_sprites.pieces[0][0], CHESS_TOTAL_PIECE_COUNT);
  Debug_Ksync(A, KSYNC_CONTINUOUS);
}

IWRAM_CODE void ChessAI_SpriteDataFromCtx(const ChessGameCtx_t *ctx) {
  const Obj_Attr_t *const CTX_OBJ_ORIGIN = &ctx->obj_data.pieces[0][0],
                   *cur_obj_addr;
  Obj_Attr_t *const LOCAL_ORIGIN = &_L_move_sprites.pieces[0][0],
             **cur_obj_addr_slot;
  Fast_Memcpy32(&_L_move_sprites, 
                CTX_OBJ_ORIGIN,
                sizeof(ChessObj_Set_t)/sizeof(WORD));
  Fast_Memcpy32(&_L_captured_sprite_tracker.ctx_compat,
                &ctx->tracker,
                sizeof(ChessAI_PieceTracker_t)/sizeof(WORD));
//  _L_captured_sprite_tracker.ctx_compat = ctx->tracker;
  _L_captured_sprite_tracker.data.dummy = NULL;  // just to be safe.
  for (u32 j,i = 0; 2>i; ++i) {
    const u32 CAPCOUNT = ctx->tracker.capcount[i];
    for (j = 0; CAPCOUNT>j; ++j) {
      cur_obj_addr = 
        *(cur_obj_addr_slot = 
            &_L_captured_sprite_tracker.data.pieces_captured[i][j]);
      *cur_obj_addr_slot =
        (&LOCAL_ORIGIN[((uptr_t)cur_obj_addr 
                        - (uptr_t)CTX_OBJ_ORIGIN)/sizeof(Obj_Attr_t)]);
    }
  }
  _L_move_sprites.sels[0].attr0.regular.disable = FALSE;
  _L_move_sprites.sels[1].attr0.regular.disable = FALSE;
}

void UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(int sel_idx, ChessBoard_Idx_t coord) {
  UPDATE_PIECE_SPRITE_LOCATION(&_L_move_sprites.sels[sel_idx], coord);
  OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS+sel_idx], 
           &_L_move_sprites.sels[sel_idx],
           1);
}
#else
#ifdef _AI_VISUALIZE_MOVE_CANDIDATES_
void UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(int sel_idx, ChessBoard_Idx_t coord) {
  UPDATE_PIECE_SPRITE_LOCATION(&_L_sels[sel_idx], coord);
  OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS+sel_idx], 
           &_L_sels[sel_idx],
           1);
}
#endif  /* _AI_VISUALIZE_MOVE_CANDIDATES_ */
#define ChessAI_ResetPieceVisualizer(dummy_field)
#define ChessAI_SearchVisualize_Move(dummy_field0, dummy_field1, dummy_field2)
#endif  /* defined(_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_) */

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
#ifdef _AI_VISUALIZE_MOVE_CANDIDATES_
  Fast_Memcpy32(&_L_sels[0], &SEL_INITIALIZER, sizeof(Obj_Attr_t)/sizeof(WORD));
  Fast_Memcpy32(&_L_sels[1], &_L_sels[0], sizeof(Obj_Attr_t)/sizeof(WORD));
#endif  /* _AI_VISUALIZE_MOVE_CANDIDATES_ */
}

IWRAM_CODE void ChessAI_Move(ChessAI_Params_t *ai_params,
                             ChessAI_MoveSearch_Result_t *returned_move) {
  const u32 ini_depth = ai_params->depth;
#ifdef _DEBUG_BUILD_
   *returned_move
        = ChessAI_ABSearch(ai_params,
                           INT16_MIN,
                           INT16_MAX); 
#else
  REG_IME = 0;
  REG_IE &= ~IRQ_FLAGS(VBLANK, KEYPAD);
  REG_IME = 1;
  *returned_move
        = ChessAI_ABSearch(ai_params,
                           INT16_MIN,
                           INT16_MAX);
  REG_IME = 0;
  REG_IF |= IRQ_FLAGS(VBLANK, KEYPAD);
  REG_IE |= IRQ_FLAGS(VBLANK, KEYPAD);
  REG_IME = 1;
#endif
  ai_params->depth = ini_depth;
  ++ai_params->gen;
}



IWRAM_CODE ChessAI_MoveSearch_Result_t ChessAI_ABSearch(
                                                      ChessAI_Params_t *params,
                                                      i16 alpha,
                                                      i16 beta) {
  ENSURE_STACK_SAFETY();

    // 1. Check transposition table
  TTableEnt_t tt_entry = {
    .key = params->root_state->zobrist,
    .depth = params->depth,
    .gen = params->gen,
    .best_move = {
      .dst.raw=0xFF,
      .start.raw = 0xFF,
    }
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
            ALLIED_KING = 
              TEAM_PIECE_IDXS_OFS|KING,
            OPP_IDX_OFS = 
              PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT^TEAM_PIECE_IDXS_OFS;
  Move_Validation_Flag_e last_move = params->last_move;
  i16 best_move = IS_MAXIMIZING(PREMOVE_ROOT_STATE->state.side_to_move)
                        ? INT16_MIN
                        : INT16_MAX;
  ChessBoard_Idx_Compact_t src;
  BOOL prune=FALSE, skip_castles = FALSE;
  params->root_state = &move_applied_state;  // switch out params board state 
                                             // ptr to the addr of the mutable 
                                             // local board state.
  for (u32 i,i_base = 0; CHESS_TEAM_PIECE_COUNT>i_base; ++i_base) {
    i = i_base|TEAM_PIECE_IDXS_OFS;
    if (!CHESS_ROSTER_PIECE_ALIVE(PREMOVE_ROOT_STATE->graph.roster, i))
      continue;

    v = PREMOVE_ROOT_STATE->graph.vertices[i];

    src = v.location;
#ifdef _AI_VISUALIZE_MOVE_CANDIDATES_
    if (MAX_DEPTH==params->depth) {
      UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(0, 
                                       BOARD_IDX_CONVERT(src, NORMAL_IDX_TYPE));
    }
#endif

    ChessMoveIterator_Alloc(&movegen, BOARD_IDX_CONVERT(src, NORMAL_IDX_TYPE),
                            PREMOVE_ROOT_STATE,
                            MV_ITER_MOVESET_ALL_MINUS_ALLIED_COLLISIONS
                               |MV_ITER_MOVESET_ORDERED_FLAGBIT);
    while (ChessMoveIterator_HasNext(&movegen)) {
      // copy immutable initial root state to the local mutable root state, 
      // so that it's ready to have the move applied to it.
#ifndef _AI_VISUALIZE_MOVE_CANDIDATES_
      UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(0, 
                                       BOARD_IDX_CONVERT(src, NORMAL_IDX_TYPE));
#endif  /* NDEF _AI_VISUALIZE_MOVE_CANDIDATES_ */
      Fast_Memcpy32(&move_applied_state,
                    PREMOVE_ROOT_STATE,
                    sizeof(BoardState_t)/sizeof(WORD));
      ChessMoveIterator_Next(&movegen, &move);
#ifdef _AI_VISUALIZE_MOVE_CANDIDATES_
      if (MAX_DEPTH==params->depth) {
        UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(1, move.dst);
      }
#else
      UPDATE_MOVE_TRAVERSAL_SEL_SPRITE(1, move.dst);
      Debug_Ksync(A, KSYNC_CONTINUOUS);
#endif  /* _AI_VISUALIZE_MOVE_CANDIDATES_ */
      // 4. Validate move special cases
      if (KING==i_base) {
        if (MOVE_CASTLE_MOVE_FLAGS_MASK&move.special_flags) {
          if (skip_castles)
            continue;
          BoardState_CastleLegalityStatus_t
            stat = BoardState_Validate_CastleLegaility(&move_applied_state,
                                                       move.dst);
          if (0>stat) {
            if (stat!=BOARD_STATE_CASTLE_BLOCKED_BY_CHECK)
              continue;
            skip_castles = TRUE;
            continue;
          }
          assert(BOARD_STATE_CASTLE_OK==stat);
        }
      }
      // 4. Apply move

      BoardState_ApplyMove(&move_applied_state, &move, src);
      if (BoardState_KingInCheck(&move_applied_state,
                                 ALLIED_KING,
                                 OPP_IDX_OFS))
        continue;

      ChessAI_SearchVisualize_Move(&move_applied_state, &move, i);

      // recursed, so now all we need to do 
      --params->depth;
      params->last_move = move.special_flags;
      {
        ChessAI_MoveSearch_Result_t mv = ChessAI_ABSearch(params,
                                              alpha,
                                              beta);
        ChessAI_ResetPieceVisualizer(PREMOVE_ROOT_STATE);
        // 6. Reset params->depth to this call's param values.
        // Don't need to reset last_move
        ++params->depth;
       
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

IWRAM_CODE i16 Piece_Development_Eval(ChessPiece_Roster_Id_e rid) {
  switch (rid) {
  case ROOK0:
  case ROOK1:
    return 0;
  case KNIGHT0:
  case KNIGHT1:
    return 100;
  case BISHOP0:
  case BISHOP1:
    return 200;
  case QUEEN:
    return 50;
  case KING:
    return 0;
  case PAWN0:
  case PAWN1:
  case PAWN2:
    return 20;
  case PAWN3:
  case PAWN4:
    return 200;
  case PAWN5:
  case PAWN6:
  case PAWN7:
    return 20;
    break;
  default:
    assert((rid&PIECE_ROSTER_ABS_ID_MASK) == rid);
  }
  return 0;
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

IWRAM_CODE i16 BoardState_Eval(const BoardState_t *state, 
           __INTENT__(UNUSED) Move_Validation_Flag_e last_move) {

  PieceState_Graph_Vertex_t v;
  const PieceState_Graph_Vertex_t *vertices = state->graph.vertices;

  const ChessPiece_Roster_t ROSTER_STATE = state->graph.roster;
  const ChessPiece_e (*board)[CHESS_BOARD_ROW_COUNT] = state->board;
  const u16 NONFORFEITED_CASTLE_FLAGS = NON_FORFEITED_CASTLE_RIGHTS(state);
  i16 score=0, base, tactical, white_check_count = 0, black_check_count = 0;
  ChessPiece_e piece;
  for (u32 j, i=0; CHESS_TOTAL_PIECE_COUNT>i; ++i) {
    if (!CHESS_ROSTER_PIECE_ALIVE(ROSTER_STATE, i))
      continue;
    v=vertices[i];
    if (i&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT) {
      if (v.edges.all&(1<<BLACK_ROSTER_ID(KING))) {
        int counter_count = 0;
        for (j=0; CHESS_TEAM_PIECE_COUNT>j; ++j) {
          if (vertices[BLACK_ROSTER_ID(j)].edges.all&(1<<i)) {
            ++counter_count;
          }
        }
        black_check_count += 50 - (counter_count*50)/CHESS_TEAM_PIECE_COUNT;
      }
    } else if (v.edges.all&(1<<WHITE_ROSTER_ID(KING))) {
      int counter_count = 0;
      for (j=0; CHESS_TEAM_PIECE_COUNT>j; ++j) {
        if (vertices[BLACK_ROSTER_ID(j)].edges.all&(1<<i)) {
          ++counter_count;
        }
      }
      white_check_count += 50 - (counter_count*50)/CHESS_TEAM_PIECE_COUNT;
    }

    piece = board[BOARD_IDX(v.location)];
    base = Piece_Eval(PIECE_IDX_MASK&piece);
    tactical=0;
    tactical+=10*v.attacking_count;
    tactical+=5*v.defending_count;
    tactical+=Positional_Eval(PIECE_IDX_MASK&piece, v.location);
    score += WHITE_FLAGBIT&piece ? (base+tactical)
                      : -(base+tactical);
  }

  assert((0==white_check_count) || (0==black_check_count));
  if (white_check_count) {
    score -= white_check_count;
  } else if (black_check_count) {
    score += black_check_count;
  }

  for (u16 flag=1; flag&ALL_CASTLE_RIGHTS_MASK; flag<<=1) {
    /* Truth table:
     *  curflag HIGH | white flag | positive score change
     *       0       |   0        |    1        // black forfeited right  (+)
     *       0       |   1        |    0        // white forfeited right  (-)
     *       1       |   0        |    0        // black has or used right (-)
     *       1       |   1        |    1        // white has or used right (+)
     * Truth table shows that positive score change can be represented by
     * <curflag is HIGH in state->castle_rights> XNOR <curflag is white>
     * AKA
     * (!((0!=(NONFORFEITED_CASTLE_FLAGS&flag))
     *          ^(0!=(WHITE_CASTLE_RIGHTS_MASK))))
     * So if we do:
     * ((0!=(NONFORFEITED_CASTLE_FLAGS&flag)) ^ (0!=(WHITE_CASTLE_MASK&flag)))
     * which is <has said castle right> XOR <flag is a white castle right>
     * we get inverse of positive score change (so negative score change):
    */
    if ((0!=(NONFORFEITED_CASTLE_FLAGS&flag))
              ^(0!=(WHITE_CASTLE_RIGHTS_MASK&flag)))
      score -= 1000;
    else
      score += 1000;
  }

  return score;

}
