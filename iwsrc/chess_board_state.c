/** (C) 12 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include <GBAdev_functions.h>
#include <assert.h>
#include <stdint.h>
#include "chess_board_state.h"
#include "GBAdev_util_macros.h"
#include "chess_board.h"
#include "chess_move_iterator.h"
#include "chess_ai.h"
#include "debug_io.h"
#include "graph.h"
#include "zobrist.h"

struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};

static const u32 CASTLE_FLAGS_CORRESPONDING_ROOK_RIDS[CASTLE_FLAG_COUNT] = {
  WHITE_ROSTER_ID(ROOK1),
  WHITE_ROSTER_ID(ROOK0),
  BLACK_ROSTER_ID(ROOK1),
  BLACK_ROSTER_ID(ROOK0)
};

#define BOARD_STATE_COUNT 4U
static IWRAM_BSS BoardState_t g_board_states[BOARD_STATE_COUNT]={0};
static EWRAM_BSS BOOL g_board_state_occupied[BOARD_STATE_COUNT]={0};

static EWRAM_CODE BoardState_t *Graph_FromCtx(BoardState_t *board_state, 
                                           const ChessGameCtx_t *ctx);
static EWRAM_CODE BOOL En_Passent_Possible(const ChessBoard_t board,
                                           u32 side_to_move,
                                           ChessBoard_File_e ep_file);
static EWRAM_CODE BoardState_t *BoardState_UpdateGraphEdges(
                                                    BoardState_t *board_state);

static EWRAM_CODE void BoardState_UpdateZobristKey(BoardState_t *board_state);
EWRAM_CODE BOOL En_Passent_Possible(const ChessBoard_t board, 
                                         u32 side_to_move,
                                         ChessBoard_File_e ep_file) {
  // Repurposing/recycling NO_VALID_EN_PASSENT_FILE to just act as a standin
  // to represent invalid file, which we will leave for one or the other 
  // if file is on edge of board, as we will check in these ifs here.
  ChessBoard_File_e files_to_check[2] = {
    NO_VALID_EN_PASSENT_FILE,
    NO_VALID_EN_PASSENT_FILE
  };
  // If white is moving, then row must be row black pawn double square moves to,
  // which is ROW_5, otherwise it's black's turn and row must be row white
  // double square advances to, which is ROW_4
  const ChessBoard_Row_e EP_ROW = (WHITE_TO_MOVE_FLAGBIT&side_to_move)
                              ? ROW_5
                              : ROW_4;
  const ChessPiece_e POTENTIAL_ATTACKER
                = CONVERT_BOARD_STATE_MOVE_FLAG(side_to_move)|PAWN_IDX;
  if (NO_VALID_EN_PASSENT_FILE==ep_file)
    return FALSE;
  // assert state is valid by making sure either ep_file is equal to the
  // invalid specifier value OR ep_file is a valid board file.
  assert((ep_file&VALID_FILE_MASK)==ep_file);

  // If EXISTS(file to the left of ep_file):
  if (FILE_A < ep_file) {
    files_to_check[0] = ep_file-1;
  }
  // Now same logic, but for right hand side of file.
  if (FILE_H > ep_file) {
    files_to_check[1] = ep_file+1;
  }
  for (int curfile, i = 0; 2 > i; ++i) {
    curfile = files_to_check[i];
    if (NO_VALID_EN_PASSENT_FILE==curfile)
      continue;
    // check if the square on same row of that file has a pawn moving side can
    // use for an en passent attack.
    if (POTENTIAL_ATTACKER==board[EP_ROW][curfile])
      return TRUE;
  }
  return FALSE;
}

EWRAM_CODE BoardState_t *BoardState_Alloc(void) {
  for (u32 i = 0; BOARD_STATE_COUNT > i; ++i)
    if (!g_board_state_occupied[i]) {
      g_board_state_occupied[i] = TRUE;
      return &g_board_states[i];
    }
  return NULL;
}



EWRAM_CODE void BoardState_Dealloc(BoardState_t *board_state) {
  size_t idx
    = ((uintptr_t)board_state - (uintptr_t)g_board_states)/sizeof(BoardState_t);

  assert(BOARD_STATE_COUNT>idx);
  assert(&g_board_states[idx] == board_state);
  static_assert(0==(sizeof(BoardState_t)%sizeof(WORD)));
  Fast_Memset32(board_state, 0, sizeof(BoardState_t)/sizeof(WORD));
  g_board_state_occupied[idx]=FALSE;
}

EWRAM_CODE BoardState_t *BoardState_FromCtx(BoardState_t *board_state,
                                            const ChessGameCtx_t *ctx) {
  if (NULL==board_state || NULL==ctx)
    return NULL;

  u64 zobrist_key=0;
  PGN_Move_t last_mv = {  /* Give last_mv invalid init state */
    .move = {
      {.raw=INVALID_IDX_RAW_VAL},
      {.raw=INVALID_IDX_RAW_VAL}
    }, 
    .move_outcome=MOVE_UNSUCCESSFUL,
    .roster_id = 0xFFFFU,
    .promotion = 0xFFFFFFFFU,
  };
  GameState_t *state = &board_state->state;
  const PGN_Round_LL_t *move_hist = &ctx->move_hist;
  const PGN_Round_t *round;
  u8 castle_rights;
  u8 whose_move;
  static_assert(0==(sizeof(ChessBoard_t)%sizeof(WORD)));
  Fast_Memcpy32(board_state->board,
                ctx->board_data,
                sizeof(ChessBoard_t)/sizeof(WORD));
  whose_move = state->side_to_move = CONVERT_CTX_MOVE_FLAG(ctx->whose_turn);
  assert(SIDE_TO_MOVE_MASK&whose_move);
  if (WHITE_TO_MOVE_FLAGBIT&whose_move) {
    state->fullmove_number = move_hist->nmemb+1;
    if (0!=move_hist->nmemb) {
      last_mv = move_hist->tail->data.moves[1];
    }
  } else {
    state->fullmove_number = move_hist->nmemb;
    // Can rest assured it move_hist will have a tail here because white move
    // always appends new tail, and black always proceeds white move.
    last_mv = move_hist->tail->data.moves[0];
  }
  if (MOVE_PAWN_TWO_SQUARE&last_mv.move_outcome) {
    state->ep_file = (u8)last_mv.move[0].coord.x;

    state->halfmove_clock = 0;
  } else if (INVALID_IDX.raw!=last_mv.move[0].raw) {
    state->ep_file = NO_VALID_EN_PASSENT_FILE;
    if (MOVE_CAPTURE&last_mv.move_outcome)
      state->halfmove_clock = 0;
    else if (0!=last_mv.promotion)
      state->halfmove_clock = 0;
    else if (PAWN_IDX
              == (PIECE_IDX_MASK & ctx->board_data[BOARD_IDX(last_mv.move[1])]))
      state->halfmove_clock = 0;
    else
      ++(state->halfmove_clock);
  }
  if (0==move_hist->nmemb) {
    state->castle_rights = castle_rights = ALL_CASTLE_RIGHTS_MASK;
  } else {
    /* Active low for now, makes deactivating during forfeiture check easier */
    castle_rights = 0;
 
    for (const PGN_Round_LL_Node_t *const END 
                                    = WHITE_TO_MOVE_FLAGBIT&whose_move
                                          ? NULL
                                          : move_hist->tail,
                                   *node = move_hist->head;
         END!=node; node = node->next) {
 
      round = &node->data;
 
      for (int i = 0, shamt; 2 > i; ++i) {
        shamt = i<<1;
        switch ((ChessPiece_Roster_Id_e)round->moves[i].roster_id) {
        case ROOK0:
          castle_rights|=QUEENSIDE_SHAMT_INVARIANT<<shamt;
          break;
        case KING:
          castle_rights|=(QUEENSIDE_SHAMT_INVARIANT
                          |KINGSIDE_SHAMT_INVARIANT)<<shamt;
          break;
        case ROOK1:
          castle_rights|=KINGSIDE_SHAMT_INVARIANT<<shamt;
          break;
        default:
          continue;
        }
      }
    }
    if (BLACK_TO_MOVE_FLAGBIT&whose_move) {
      round = &move_hist->tail->data;
      switch ((ChessPiece_Roster_Id_e)round->moves[0].roster_id) {
        case ROOK0:
          castle_rights
            |=QUEENSIDE_SHAMT_INVARIANT<<CASTLE_RIGHTS_WHITE_FLAGS_SHAMT;
          break;
        case ROOK1:
          castle_rights
            |=KINGSIDE_SHAMT_INVARIANT<<CASTLE_RIGHTS_WHITE_FLAGS_SHAMT;
          break;
        case KING:
          castle_rights
            |=(KINGSIDE_SHAMT_INVARIANT
                |QUEENSIDE_SHAMT_INVARIANT)
                              <<CASTLE_RIGHTS_WHITE_FLAGS_SHAMT;
          break;
        default:
          break;
      }
    }
    for (u32 roster_id, i = 0; CASTLE_FLAG_COUNT>i; ++i) {
      if ((1U<<i)&castle_rights) {
        // remember its still active low, so if condition yields true,
        // that means that flag's corresponding castle rights have already
        // been forfeit, so continue.
        continue;
      }
      roster_id = CASTLE_FLAGS_CORRESPONDING_ROOK_RIDS[i];
      if (CHESS_ROSTER_PIECE_ALIVE(ctx->tracker.roster, roster_id))
        continue;
      castle_rights|=(1U<<i);

    }

    /* Now it's active high. Set castle_rights^=ZOBRIST_CASTLE_ID_MASK to clear
     * only relevant bits that were high, and that should allow us to ensure 
     * that castle_rights = ZOBRIST_CASTLE_ID_MASK&ACTIVE_HIGH(castle_rights).
     * so then we set state->castle_rights = castle_rights
     */
    castle_rights ^= ZOBRIST_CASTLE_ID_MASK;
    state->castle_rights = castle_rights;
  }
  // assert castle_rights is repping a valid state
  assert((castle_rights&ZOBRIST_CASTLE_ID_MASK) == castle_rights);

  for (u32 curpiece, file, row=ROW_8; CHESS_BOARD_ROW_COUNT>row; ++row) {
    for (file=FILE_A; CHESS_BOARD_FILE_COUNT>file; ++file) {
      curpiece = ctx->board_data[row][file];
      if (EMPTY_IDX==curpiece)
        continue;
      zobrist_key^=BOARD_ZKEY_ENTS(zobrist_table)[row]
                                    [file]
                                    [ZID_FROM_BOARD_PIECE_DATA(curpiece)];
    }
  }
  if (BLACK_TO_MOVE_FLAGBIT&whose_move)
    zobrist_key^=SIDE_TO_MOVE_ZKEY_ENT(zobrist_table);
  zobrist_key^=CASTLE_ZKEY_ENTS(zobrist_table)[castle_rights];
  if (En_Passent_Possible(ctx->board_data, whose_move, state->ep_file))
    zobrist_key^=EN_PASSENT_ZKEY_ENTS(zobrist_table)[state->ep_file];
  board_state->zobrist = zobrist_key;
  
  return Graph_FromCtx(board_state, ctx);
}

EWRAM_CODE BoardState_t *Graph_FromCtx(BoardState_t *board_state, 
                                    const ChessGameCtx_t *ctx) {
  ChessMoveIteration_t mv;
  ChessPiece_Data_t hit_piece_query_obj = {0};
  
  ChessMoveIterator_t iterator;
  const ChessBoard_Row_t *board_data = ctx->board_data;
  const Graph_t *ctx_graph = ctx->tracker.piece_graph;
  const GraphNode_t *ctx_vert;
  PieceState_Graph_t edgeless_graph;
  PieceAdjacencyFields_t cur_adj;
  ChessBoard_Idx_t loc;
  ChessBoard_Idx_Compact_t loc_compact;
  ChessPiece_Roster_t roster = edgeless_graph.roster = ctx->tracker.roster;
  // Make sure that a Fast_Memset32 on the graph's vertex hashmap yields a full
  // write of buffer, without any stragglers (i.e.: its size is word-aligned)
  static_assert(0==
      (sizeof(u8[CHESS_BOARD_ROW_COUNT][CHESS_BOARD_FILE_COUNT])%sizeof(WORD)));
  static_assert(UINT32_MAX==PIECE_GRAPH_HASHMAP_INITIALIZER_WORD);
  Fast_Memset32(edgeless_graph.vertex_hashmap,
                PIECE_GRAPH_HASHMAP_INITIALIZER_WORD,
                sizeof(u8[CHESS_BOARD_ROW_COUNT][CHESS_BOARD_FILE_COUNT])
                          / sizeof(WORD));
  
  
  for (u32 total_cardinality,
           w_cardinality,
           b_cardinality,
           hit_idx,
           i = 0; 
       CHESS_TOTAL_PIECE_COUNT>i; 
       ++i) {
    if (!CHESS_ROSTER_PIECE_ALIVE(roster, i))
      continue;
    ctx_vert = &ctx_graph->vertices[i];
    assert(NULL!=ctx_vert->data);
    loc = ((const ChessPiece_Data_t*)ctx_vert->data)->location;
    edgeless_graph.vertex_hashmap[BOARD_IDX(loc)] = i&PIECE_ROSTER_ABS_ID_MASK;
    static_assert(sizeof(cur_adj.all) == sizeof(cur_adj));
    cur_adj.all = 0;
    loc_compact = BOARD_IDX_CONVERT(loc, COMPACT_IDX_TYPE);
     
    assert(ChessMoveIterator_Alloc(&iterator, loc, board_state, MV_ITER_MOVESET_COLLISIONS_ONLY_SET));
    for (BOOL hasnext = ChessMoveIterator_Next(&iterator, &mv);
         hasnext;
         hasnext = ChessMoveIterator_Next(&iterator, &mv)) {
//      assert(ChessMoveIterator_Next(&iterator, &mv));
      assert(EMPTY_IDX!=board_data[BOARD_IDX(mv.dst)]);
      hit_piece_query_obj.location = mv.dst;
      ctx_vert = Graph_Get_Vertex(ctx_graph, &hit_piece_query_obj);
      assert(NULL!=ctx_vert);
      assert(CHESS_ROSTER_PIECE_ALIVE(roster, ctx_vert->idx));
      assert(NULL!=ctx_vert->data);
      hit_idx = ctx_vert->idx;
      cur_adj.all |= 1<<hit_idx;
    }
    ChessMoveIterator_Dealloc(&iterator);
    
    w_cardinality = __builtin_popcount(cur_adj.team_invariant.white);
    b_cardinality = __builtin_popcount(cur_adj.team_invariant.black);
    total_cardinality = w_cardinality+b_cardinality;
    if (i&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT) {
      edgeless_graph.vertices[i]
        = ((PieceState_Graph_Vertex_t) {
          .location = loc_compact,
          .edges = cur_adj,
          .attacking_count = b_cardinality,
          .defending_count = w_cardinality,
          .total_edge_count = total_cardinality
        });
    } else {
      edgeless_graph.vertices[i]
        = ((PieceState_Graph_Vertex_t) {
          .location = loc_compact,
          .edges = cur_adj,
          .attacking_count = w_cardinality,
          .defending_count = b_cardinality,
          .total_edge_count = total_cardinality
        });
    }
  }
  board_state->graph = edgeless_graph;
  return board_state;
}

EWRAM_CODE BoardState_t *BoardState_ApplyMove(BoardState_t *board_state,
                                        const ChessBoard_Idx_Compact_t move[2],
                                        Move_Validation_Flag_e flags) {
  // Update graph.hashmap[move[0]] = PIECE_GRAPH_EMPTY_HASHENT
  // Update graph.hashmap[move[1]] = moving_idx
  // Update graph.vertices[moving_idx].location = move[1], but only after
  // lazy deleting of captured piece if there is one at move[1]
  u32 moving_idx = board_state->graph.vertex_hashmap[BOARD_IDX(move[0])];
  ChessPiece_e moving_piece = board_state->board[BOARD_IDX(move[0])],
               moving_side = (moving_idx&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT
                  ? WHITE_FLAGBIT
                  : BLACK_FLAGBIT), moving_piece_type;
  moving_piece_type = moving_piece&PIECE_IDX_MASK;
  assert(moving_side&moving_piece);
  board_state->graph.vertex_hashmap[BOARD_IDX(move[0])]
    = PIECE_GRAPH_EMPTY_HASHENT;
  assert (PIECE_GRAPH_EMPTY_HASHENT != moving_idx);
  if (MOVE_CAPTURE&flags) {
    ChessBoard_Idx_t capt_loc = BOARD_IDX_CONVERT(move[1], NORMAL_IDX_TYPE);
    u32 captured_idx;
    u8 *captured_vertex_hashent;
    ChessPiece_e captured_piece;

    if (MOVE_EN_PASSENT&flags) {
      capt_loc.coord.y = move[0].coord.y;
    }
    captured_vertex_hashent
      = &board_state->graph.vertex_hashmap[BOARD_IDX(capt_loc)];
    captured_piece = board_state->board[BOARD_IDX(capt_loc)];
    assert((moving_side^PIECE_TEAM_MASK)&captured_piece);
    captured_idx = *captured_vertex_hashent;
    if (MOVE_EN_PASSENT&flags) {
      *captured_vertex_hashent = PIECE_GRAPH_EMPTY_HASHENT;
      assert(((moving_side^PIECE_TEAM_MASK)|PAWN_IDX)==captured_piece);
    }
    assert(PIECE_GRAPH_EMPTY_HASHENT!=captured_idx);
    
    assert((PIECE_ROSTER_ABS_ID_MASK&captured_idx)==captured_idx);
    assert(board_state->graph.roster.all&(1<<captured_idx));
    board_state->graph.roster.all^=(1<<captured_idx);
    // Lazy deletion therefore dont bother with clearing anything in 
    // captured vertex
    board_state->board[BOARD_IDX(capt_loc)] = EMPTY_IDX;
  } else if (MOVE_CASTLE_MOVE_FLAGS_MASK&flags) {
    // Update graph.hashmap[rook_start_loc] = PIECE_GRAPH_EMPTY_HASHENT
    // Update graph.hashmap[rook_end_loc] = rook_idx
    // Update graph.vertices[rook_idx].location = rook_end_loc
    PieceState_Graph_Vertex_t *rook_vertex;
    u32 rook_idx, castle_rights_flagbit;
    ChessPiece_e rook_piece = ROOK_IDX;
    ChessBoard_Idx_Compact_t rook_start, rook_end;
    if (MOVE_CASTLE_QUEENSIDE&flags) {
      rook_start.coord.x = FILE_A;
      rook_end.coord.x = FILE_D;
      rook_idx = ROOK0;
      castle_rights_flagbit = QUEENSIDE_SHAMT_INVARIANT;
    } else {
      rook_start.coord.x = FILE_H;
      rook_end.coord.x = FILE_F;
      rook_idx = ROOK1;
      castle_rights_flagbit = KINGSIDE_SHAMT_INVARIANT;
    }
    if (moving_idx&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT) {
      board_state->state.castle_rights ^=(WK|WQ);
      rook_idx |= PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
      rook_start.coord.y = rook_end.coord.y = ROW_1;
      castle_rights_flagbit <<= CASTLE_RIGHTS_WHITE_FLAGS_SHAMT;
      rook_piece |= WHITE_FLAGBIT;
    } else {
      board_state->state.castle_rights ^= (BK|BQ);
      rook_start.coord.y = rook_end.coord.y = ROW_8;
      castle_rights_flagbit <<= CASTLE_RIGHTS_BLACK_FLAGS_SHAMT;
      rook_piece |= BLACK_FLAGBIT;
    }
    assert(0!=(board_state->state.castle_rights&castle_rights_flagbit));
    rook_vertex = &board_state->graph.vertices[rook_idx];
    assert(((const u8)board_state->graph.vertex_hashmap[BOARD_IDX(rook_start)])
                            ==rook_idx);
    board_state->graph.vertex_hashmap[BOARD_IDX(rook_start)]
      = PIECE_GRAPH_EMPTY_HASHENT;
    board_state->graph.vertex_hashmap[BOARD_IDX(rook_end)]
      = rook_idx;

    assert((const u8)rook_vertex->location.raw == rook_start.raw);
    rook_vertex->location = rook_end;
    assert((const ChessPiece_e)board_state->board[BOARD_IDX(rook_start)]
                    ==rook_piece);
    assert((const ChessPiece_e)board_state->board[BOARD_IDX(rook_end)]
                    ==EMPTY_IDX);
    board_state->board[BOARD_IDX(rook_end)] = rook_piece;
    board_state->board[BOARD_IDX(rook_start)] = EMPTY_IDX;
    
  }
  // Now that captured piece is taken care of, we can update hashmap[move[1]]
  // safely.
  board_state->graph.vertex_hashmap[BOARD_IDX(move[1])] = moving_idx;
  board_state->board[BOARD_IDX(move[1])] = moving_piece;
  ensure(board_state->graph.vertices[moving_idx].location.raw==move[0].raw,
    "state graph says location of vertex = "
    "{.raw=\x1b[0x44E4]0x%02hhX\x1b[0x1484]}.\nMove idxs are:\n\t"
    "[0] = {.raw=\x1b[0x44E4]0x%02hhX\x1b[0x1484]},\t[1] = "
    "{.raw=\x1b[0x44E4]0x%02hhX\x1b[0x1484]}",
    board_state->graph.vertices[moving_idx].location.raw,
    move[0].raw, move[1].raw);
  board_state->graph.vertices[moving_idx].location = move[1];
  board_state->board[BOARD_IDX(move[0])] = EMPTY_IDX;
  do {
    int shamt = (moving_idx&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT)
                            ? CASTLE_RIGHTS_WHITE_FLAGS_SHAMT
                            : CASTLE_RIGHTS_BLACK_FLAGS_SHAMT,
        team_invariant_mvidx = moving_idx&PIECE_ROSTER_ABS_ID_MASK;
    // If all castle rights already forfeit, no point in doing this
    if (0==(board_state->state.castle_rights
          &(CASTLE_RIGHTS_SHAMT_INVARIANT<<shamt)))
      break;
    if (KING_IDX!=moving_piece_type && ROOK_IDX!=moving_piece_type)
      break;
    if (PAWN0<=team_invariant_mvidx)
      break;
    if (KING_IDX==moving_piece_type)
      assert(KING==team_invariant_mvidx);
    else
      assert(ROOK_IDX==moving_piece_type 
          && (ROOK0==team_invariant_mvidx || ROOK1==team_invariant_mvidx));
    if (ROOK0==moving_idx) {
      board_state->state.castle_rights&=~(QUEENSIDE_SHAMT_INVARIANT<<shamt);
    } else if (ROOK1==team_invariant_mvidx) {
      board_state->state.castle_rights&=~(KINGSIDE_SHAMT_INVARIANT<<shamt);
    } else {
      assert(KING==team_invariant_mvidx);
      board_state->state.castle_rights
        &=~((KINGSIDE_SHAMT_INVARIANT|QUEENSIDE_SHAMT_INVARIANT)<<shamt);
    }
  } while (0);
  if (MOVE_PAWN_TWO_SQUARE&flags) {
    ChessBoard_Idx_Compact_t adjs[2] = {
      INVALID_IDX_COMPACT,
      INVALID_IDX_COMPACT
    };
    const ChessPiece_e ENEMY_PAWN = PAWN_IDX|(moving_side^PIECE_TEAM_MASK);

    if (FILE_A<move[0].coord.x) {
      adjs[0] = move[0];
      --adjs[0].coord.x;
    }
    if (FILE_H>move[0].coord.x) {
      adjs[1] = move[0];
      ++adjs[1].coord.x;
    }
    board_state->state.ep_file = NO_VALID_EN_PASSENT_FILE;
    for (int i = 0; 2>i;++i) {
      if (INVALID_IDX_COMPACT_RAW==adjs[i].raw)
        continue;
      if (ENEMY_PAWN!=board_state->board[BOARD_IDX(adjs[i])])
        continue;
      board_state->state.ep_file = move[0].coord.x;
      break;
    }
  } else {
    board_state->state.ep_file = NO_VALID_EN_PASSENT_FILE;
  }
  if (BLACK_FLAGBIT==moving_side)  
    ++board_state->state.fullmove_number;
  if (PAWN_IDX==moving_piece_type || MOVE_CAPTURE&flags)
    board_state->state.halfmove_clock = 0;
  else
    ++board_state->state.halfmove_clock;
  board_state->state.side_to_move ^= SIDE_TO_MOVE_MASK;
  BoardState_UpdateZobristKey(board_state);

  return BoardState_UpdateGraphEdges(board_state);
  
}

EWRAM_CODE void BoardState_UpdateZobristKey(BoardState_t *board_state) {
  u64 zobrist_key = 0ULL;
  u32 whose_move =board_state->state.side_to_move,
      castle_rights = board_state->state.castle_rights,
      ep_file = board_state->state.ep_file;
  // assert castle_rights is repping a valid state
  assert((castle_rights&ZOBRIST_CASTLE_ID_MASK) == castle_rights);
  for (u32 curpiece, file, row=ROW_8; CHESS_BOARD_ROW_COUNT>row; ++row) {
    for (file=FILE_A; CHESS_BOARD_FILE_COUNT>file; ++file) {
      curpiece = board_state->board[row][file];
      if (EMPTY_IDX==curpiece)
        continue;
      zobrist_key^=BOARD_ZKEY_ENTS(zobrist_table)[row]
                                    [file]
                                    [ZID_FROM_BOARD_PIECE_DATA(curpiece)];
    }
  }
  ;
  if (BLACK_TO_MOVE_FLAGBIT&whose_move)
    zobrist_key^=SIDE_TO_MOVE_ZKEY_ENT(zobrist_table);
  zobrist_key^=CASTLE_ZKEY_ENTS(zobrist_table)[castle_rights];
  if (En_Passent_Possible(board_state->board, whose_move, ep_file))
    zobrist_key^=EN_PASSENT_ZKEY_ENTS(zobrist_table)[ep_file];
  board_state->zobrist = zobrist_key;
}


EWRAM_CODE BoardState_t *BoardState_UpdateGraphEdges(
                                                   BoardState_t *board_state) {
  ChessMoveIteration_t mv_iter;
  ChessBoard_Idx_t cur_loc;
  ChessMoveIterator_t iterator;
  PieceState_Graph_Vertex_t *vertices = board_state->graph.vertices;
  const u8 (*const vmap)[CHESS_BOARD_ROW_COUNT] = board_state->graph.vertex_hashmap;
  const ChessBoard_Row_t *board = board_state->board;
  PieceAdjacencyFields_t cur_adjbits;
  u32 hit_idx;
  const ChessPiece_Roster_t roster = board_state->graph.roster;
  ChessPiece_e curpiece;
  for (u32 white_cardinality, black_cardinality, total, i = 0;
       CHESS_TOTAL_PIECE_COUNT>i; 
       ++i) {
    if (!CHESS_ROSTER_PIECE_ALIVE(roster, i))
      continue;
    cur_loc = BOARD_IDX_CONVERT(vertices[i].location, NORMAL_IDX_TYPE);
    assert(vmap[BOARD_IDX(cur_loc)]==i);
    curpiece = board[BOARD_IDX(cur_loc)];
    assert(EMPTY_IDX!=curpiece);
    assert(ChessMoveIterator_Alloc(&iterator, cur_loc, board_state, MV_ITER_MOVESET_COLLISIONS_ONLY_SET));
    cur_adjbits.all = 0;
    while (ChessMoveIterator_HasNext(&iterator)) {
      assert(ChessMoveIterator_Next(&iterator, &mv_iter));
      assert(EMPTY_IDX!=board[BOARD_IDX(mv_iter.dst)]);
      hit_idx = vmap[BOARD_IDX(mv_iter.dst)];
      assert(PIECE_GRAPH_EMPTY_HASHENT!=hit_idx);
      assert(BOARD_IDX_CONVERT(mv_iter.dst, COMPACT_IDX_TYPE).raw
                    ==vertices[hit_idx].location.raw);
      assert(CHESS_ROSTER_PIECE_ALIVE(roster, hit_idx));
      cur_adjbits.all|=(1<<hit_idx);
    }
    ChessMoveIterator_Dealloc(&iterator);
    white_cardinality = __builtin_popcount(cur_adjbits.team_invariant.white);
    black_cardinality = __builtin_popcount(cur_adjbits.team_invariant.black);
    total = white_cardinality+black_cardinality;
    vertices[i].total_edge_count = total; 
    vertices[i].edges = cur_adjbits;
    if (i&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT) {
      vertices[i].attacking_count = black_cardinality;
      vertices[i].defending_count = white_cardinality;
    } else {
      vertices[i].attacking_count = white_cardinality;
      vertices[i].defending_count = black_cardinality;
    }
  }
  return board_state;
}
