/** (C) Burt Sumner 2025 */
#include <stddef.h>

#include <GBAdev_types.h>
#include <GBAdev_functions.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>

#include "chess_board.h"
#include "chess_sprites.h"
#include "chess_obj_sprites_data.h"
#include "graph.h"
#include "linked_list.h"

extern EWRAM_CODE void Load_Chess_Sprites_8BPP(Tile8_t *dst, u16 fg_clr, u16 sel_bg_clr);

static void ChessObj_Init_Team_Pieces(ChessObj_Team_t obj_data,
                                      Graph_t *piece_graph,
                                      int team_flag);

static void ChessObj_Init_Sel_Pieces(ChessObj_Mvmt_Sel_t sel_objs);

static void ChessObj_Init_All_Pieces(ChessObj_Set_t *obj_data,
    Graph_t *piece_graph);
static void ChessBoard_Init(ChessBoard_t);




static Move_Validation_Flag_e ChessBoard_ValidateEnPassent(
                                         const ChessBoard_t board_data,
                                         const ChessBoard_Idx_t move[2],
                                         const PGN_Round_LL_t *mvmt_ll,
                                         ChessPiece_e moving_piece);

static Move_Validation_Flag_e ChessBoard_ValidateCastle(const ChessBoard_Row_t *board_data,
                                                        const ChessBoard_Idx_t *move,
                                                        const ChessPiece_Tracker_t *tracker,
                                                        const PGN_Round_LL_t *mvmt_ll);



static BOOL ChessBoard_PieceIsPinned(
                              const ChessBoard_t board_data, 
                              const ChessBoard_Idx_t *move,
                              const ChessPiece_Tracker_t *piece_tracker);


static BOOL ChessBoard_ValidateMoveAddressesCheck(
                                      const ChessGameCtx_t *ctx,
                                      const ChessPiece_Data_t *checking_pieces,
                                      u32 checking_piece_ct,
                                      Move_Validation_Flag_e move_result);
static
BOOL ChessBoard_ValidateKingMoveEvadesOpp(
                                    const ChessBoard_t board_data,
                                    const ChessBoard_Idx_t *move,
                                    const ChessPiece_Data_t *checking_piece);



static int ChessPiece_Data_Cmp_Cb(const void *a, const void *b);





void ChessGameCtx_Init(ChessGameCtx_t *ctx) {
  ctx->whose_turn = WHITE_FLAGBIT;
  ctx->tracker = (ChessPiece_Tracker_t){
      .roster = { .all = ~0 },
      .pieces_captured = {0},
      .capcount = {0},
      .piece_graph = Graph_Init(NULL,
                                NULL,
                                ChessPiece_Data_Cmp_Cb,
                                sizeof(ChessPiece_Data_t))
  };
  assert(NULL!=ctx->tracker.piece_graph);
  ChessBoard_Init(ctx->board_data);
  ChessObj_Init_All_Pieces(&(ctx->obj_data), ctx->tracker.piece_graph);
}

void ChessGameCtx_Close(ChessGameCtx_t *ctx) {
  Graph_Close(ctx->tracker.piece_graph);
  PGN_Round_LL_t *rll=&ctx->move_hist;
  LL_CLOSE(PGN_Round, rll);
  Fast_Memset32(ctx, 0, sizeof(ChessGameCtx_t)/sizeof(WORD));
}

void ChessBoard_Init(ChessBoard_t board) {
  Fast_Memset32(board,
                (EMPTY_IDX<<16)|EMPTY_IDX,
                sizeof(ChessBoard_t) / sizeof(WORD));
  for (ChessBoard_File_e file = FILE_A; file < CHESS_BOARD_FILE_COUNT; ++file) {
    file&=CHESSBOARD_FILE_ENUM_VALIDITY_MASK;
    board[ROW_8][file] = BLACK_FLAGBIT|BOARD_BACK_ROWS_INIT[file];
    board[ROW_7][file] = BLACK_PAWN;
    board[ROW_2][file] = WHITE_PAWN;
    board[ROW_1][file] = WHITE_FLAGBIT|BOARD_BACK_ROWS_INIT[file];
  }
}

void ChessObj_Init_Team_Pieces(ChessObj_Team_t obj_data, 
                               Graph_t *piece_graph, 
                               int team_flag) {
  Obj_Attr_t *obj;
  ChessBoard_Row_e row;
  ChessPiece_Data_t graph_data;
  int idx, roster_id_ofs;
  roster_id_ofs = (team_flag&WHITE_FLAGBIT) 
                    ? PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT
                    : 0;

  for (int row_ofs = 0; row_ofs < 2; ++row_ofs) {
    if (team_flag&WHITE_FLAGBIT) {
      row = ROW_1 - row_ofs;
    } else {
      row = ROW_8  + row_ofs;
    }
    for (ChessBoard_File_e file = FILE_A;
         file < CHESS_BOARD_FILE_COUNT;
         ++file) {
      idx = row_ofs*CHESS_BOARD_FILE_COUNT+file;
      obj = obj_data+idx;
      graph_data = (ChessPiece_Data_t) {
            .location = {.coord = {.x=file, .y=row}},
            .obj_ctl = obj,
            .roster_id = idx+roster_id_ofs,
      };
      Graph_Add_Vertex(piece_graph, &graph_data);
      obj->attr0.regular.disable = FALSE;
      obj->attr0.regular.pal_8bpp = TRUE;
      obj->attr0.regular.y
        = row*Chess_sprites_Glyph_Height + CHESS_BOARD_Y_OFS;

      obj->attr1.regular.obj_size = CHESS_OATTR_SIZE_VALUE;
      obj->attr1.regular.x
        = file*Chess_sprites_Glyph_Width + CHESS_BOARD_X_OFS;
      
      obj->attr2.sprite_idx = 2*TILES_PER_CSPR*BOARD_BACK_ROWS_INIT[idx];
      if (team_flag&BLACK_FLAGBIT)
        obj->attr2.sprite_idx += 2*TILES_PER_CSPR*Chess_sprites_Glyph_Count;
    }
  }

}

void ChessObj_Init_Sel_Pieces(ChessObj_Mvmt_Sel_t sel_objs) {
  for (int i = 0; i < 2; ++i) {
    sel_objs[i].attr0.raw |= ((Obj_Attr_Fields0_t){
        .regular = {
          .mode=OBJ_GFX_MODE_BLEND,
          .pal_8bpp = TRUE,
        }
    }).raw;
    sel_objs[i].attr1.raw |= ((Obj_Attr_Fields1_t){
      .regular = {
        .obj_size = CHESS_OATTR_SIZE_VALUE,
      }
    }).raw;
    sel_objs[i].attr2.priority = 1;
  }
  
}

void ChessObj_Init_All_Pieces(ChessObj_Set_t *obj_data, Graph_t *piece_graph) {
  assert(OAM_Init((Obj_Attr_t*)obj_data, sizeof(ChessObj_Set_t)/sizeof(Obj_Attr_t)));
  PAL_MEM_OBJ[0] = 0;
  PAL_MEM_OBJ[1] = WHITE_PIECE_CLR;
  PAL_MEM_OBJ[2] = BLACK_PIECE_CLR;
  PAL_MEM_OBJ[3] = VALID_SEL_SQUARE_CLR;
  PAL_MEM_OBJ[4] = INVALID_SEL_SQUARE_CLR;
  Load_Chess_Sprites_8BPP(&TILE8_MEM[4][0],
                          1, 3);
  Load_Chess_Sprites_8BPP(&TILE8_MEM[4]
                                    [TILES_PER_CSPR*Chess_sprites_Glyph_Count],
                          2, 4);

  ChessObj_Init_Team_Pieces(obj_data->pieces[0], piece_graph, BLACK_FLAGBIT);
  ChessObj_Init_Team_Pieces(obj_data->pieces[1], piece_graph, WHITE_FLAGBIT);
  ChessObj_Init_Sel_Pieces(obj_data->sels);
  

  OAM_Copy(OAM_ATTR, (Obj_Attr_t*)obj_data, sizeof(ChessObj_Set_t)/sizeof(Obj_Attr_t));
}

#define ChessBoard_ValidateMoveStartPosMatchesTurn(whose_turn, start_piece)\
  (0!=(whose_turn&start_piece))
#define ChessBoard_ValidateMoveEndPosEmptyOrOpp(whose_turn, end_piece)\
  (0==(whose_turn&end_piece) || EMPTY_IDX==(PIECE_IDX_MASK&end_piece))

Move_Validation_Flag_e ChessBoard_ValidateMoveLegality(
                                     const ChessBoard_t board_data,
                                     const ChessBoard_Idx_t move[2],
                                     const ChessPiece_Tracker_t *tracker,
                                     const PGN_Round_LL_t *mvmt_ll) {
  ChessBoard_Idx_t spos = move[0], epos = move[1];
  ChessPiece_e spiece = board_data[spos.coord.y][spos.coord.x],
               epiece = board_data[epos.coord.y][epos.coord.x];
  Move_Validation_Flag_e capture_flag;
  Mvmt_Dir_e dir = ChessBoard_MoveGetDir(move);
  int xdiff = spos.coord.x - epos.coord.x, ydiff = spos.coord.y - epos.coord.y;
  
  if (EMPTY_IDX!=(epiece&PIECE_IDX_MASK))
    capture_flag = MOVE_CAPTURE;
  else
    capture_flag = 0;


  
  switch ((ChessPiece_e)(spiece&PIECE_IDX_MASK)) {
  case PAWN_IDX: 
    {
      ydiff = (spiece&WHITE_FLAGBIT)
                  ? spos.coord.y - epos.coord.y
                  : epos.coord.y - spos.coord.y;
      if (0>ydiff)
        return MOVE_UNSUCCESSFUL;
      if (0==xdiff && 2>=ydiff) {
        if (0==ydiff || EMPTY_IDX != (epiece&PIECE_IDX_MASK)) {
          return MOVE_UNSUCCESSFUL;
        } else if (1==ydiff) {
          return MOVE_SUCCESSFUL;
        } else {
        if (spiece&WHITE_FLAGBIT) {
          if (ROW_2!=spos.coord.y)
            return MOVE_UNSUCCESSFUL;
          if (EMPTY_IDX == (board_data[spos.coord.y-1][spos.coord.x]&PIECE_IDX_MASK))
            return MOVE_SUCCESSFUL|MOVE_PAWN_TWO_SQUARE;
          return MOVE_UNSUCCESSFUL;
          } else {
            if (ROW_7!=spos.coord.y)
              return MOVE_UNSUCCESSFUL;
            if (EMPTY_IDX == (board_data[spos.coord.y+1][spos.coord.x]&PIECE_IDX_MASK))
              return MOVE_SUCCESSFUL|MOVE_PAWN_TWO_SQUARE;
            return MOVE_UNSUCCESSFUL;
          }
        }
      } else if (1==ABS(xdiff, 32) && 1==ydiff) {
        if (!capture_flag) {
          if (PIECE_TEAM_MASK == (board_data[spos.coord.y][epos.coord.x]^spiece))
            return ChessBoard_ValidateEnPassent(board_data, move, mvmt_ll, spiece);
          return MOVE_UNSUCCESSFUL;
        }
        return MOVE_SUCCESSFUL|capture_flag;
      } else {
        return MOVE_UNSUCCESSFUL;
      }
    }
    break;
  case BISHOP_IDX: 
    {
      if (DIAGONAL_MVMT_FLAGBIT==(DIAGONAL_MVMT_FLAGBIT&dir)) {
        if (1!=__builtin_popcount(dir&HOR_MASK))
          return MOVE_UNSUCCESSFUL;
        if (1!=__builtin_popcount(dir&VER_MASK))
          return MOVE_UNSUCCESSFUL;
        if (3!=__builtin_popcount(dir))
          return MOVE_UNSUCCESSFUL;
      } else {
        return MOVE_UNSUCCESSFUL;
      }
    }
    break;
  case ROOK_IDX:
    {
      if (1!=__builtin_popcount(dir) || dir!=(dir&(HOR_MASK|VER_MASK)))
        return MOVE_UNSUCCESSFUL;
        
    }
    break;
  case KNIGHT_IDX:
    {
      if (KNIGHT_MVMT_FLAGBIT==(KNIGHT_MVMT_FLAGBIT&dir)) {
        if (1!=__builtin_popcount(dir&HOR_MASK))
          return MOVE_UNSUCCESSFUL;
        if (1!=__builtin_popcount(dir&VER_MASK))
          return MOVE_UNSUCCESSFUL;
        if (3!=__builtin_popcount(dir))
          return MOVE_UNSUCCESSFUL;
      } else {
        return MOVE_UNSUCCESSFUL;
      }
    }
    return MOVE_SUCCESSFUL|capture_flag;
    break;
  case QUEEN_IDX:
    {
      if (DIAGONAL_MVMT_FLAGBIT&dir) {
        if (1==__builtin_popcount(dir&HOR_MASK) 
            && 1==__builtin_popcount(dir&VER_MASK)
            && 3==__builtin_popcount(dir))
          break;
        if (ABS(xdiff,32)==ABS(ydiff,32))
          break;
      }
      if (1==__builtin_popcount(dir) && dir==(dir&(HOR_MASK|VER_MASK)))
        break;
      return MOVE_UNSUCCESSFUL;
    }
    break;
  case KING_IDX:
    {
      int popct = __builtin_popcount(dir);
      if (0==(popct&1))
        return MOVE_UNSUCCESSFUL;
      if (3==popct) {
        if (KNIGHT_MVMT_FLAGBIT&dir)
          return MOVE_UNSUCCESSFUL;
        assert((0!=(DIAGONAL_MVMT_FLAGBIT&dir))
                &(0!=(dir&HOR_MASK))
                &(0!=(dir&VER_MASK)));
        if (1==ABS(xdiff, 32) && 1==ABS(ydiff,32))
          return MOVE_SUCCESSFUL|capture_flag;
      } else if (1==popct) {
        assert((0!=(dir&HOR_MASK))
                ^(0!=(dir&VER_MASK)));
        if (dir&VER_MASK)
          return (1==ABS(ydiff, 32) && 0==xdiff)
              ? (MOVE_SUCCESSFUL|capture_flag)
              : (MOVE_UNSUCCESSFUL);
      } else {
        return MOVE_UNSUCCESSFUL;
      }
      // Add this assertion to make sure logic flow follows expected path
      assert(dir&HOR_MASK);
      if (1==ABS(xdiff,32) && 0==ydiff) {
        return  MOVE_SUCCESSFUL|capture_flag;
      } else if (FILE_E==spos.coord.x && (FILE_C==epos.coord.x 
                                          || FILE_G==epos.coord.x)) {
        if (WHITE_FLAGBIT&spiece) {
          if (ROW_1==spos.coord.y && ROW_1==epos.coord.y)
            return ChessBoard_ValidateCastle(board_data,
                                             move,
                                             tracker,
                                             mvmt_ll);
        } else if (ROW_8==spos.coord.y && ROW_8==epos.coord.y) {
          return ChessBoard_ValidateCastle(board_data,
                                           move,
                                           tracker,
                                           mvmt_ll);
        }
      }
      return MOVE_UNSUCCESSFUL;
      break;
    }
  default: assert(0);
  }
  if (dir&INVALID_MVMT_FLAGBIT)
    return MOVE_UNSUCCESSFUL;
  if (!ChessBoard_ValidateMoveClearance(board_data, move, dir))
    return MOVE_UNSUCCESSFUL;
  else
    return MOVE_SUCCESSFUL|capture_flag;
}

Mvmt_Dir_e ChessBoard_MoveGetDir(const ChessBoard_Idx_t move[2]) {
  Mvmt_Dir_e ret = 0;
  int dx = move[1].coord.x - move[0].coord.x,
      dy = move[1].coord.y - move[0].coord.y,
      dxabs, dyabs;
  dxabs = ABS(dx, 32), dyabs = ABS(dy,32);
  if (dx) {
    ret|= (0 < dx ? RIGHT_FLAGBIT : LEFT_FLAGBIT);
  }
  if (dy) {
    ret |= (0 < dy ? DOWN_FLAGBIT : UP_FLAGBIT);
  }
  if (0==ret)
    return INVALID_MVMT_FLAGBIT;
  if (!dx || !dy) {
    return ret;
  }
  if ((const int)dxabs == dyabs) {
    ret|=DIAGONAL_MVMT_FLAGBIT;
  } else if ((1==dxabs && 2==dyabs) || (2==dxabs && 1==dyabs)) {
    ret|=KNIGHT_MVMT_FLAGBIT;
  } else {
    return INVALID_MVMT_FLAGBIT;
  }
  return ret;
}



BOOL ChessBoard_PieceIsPinned(const ChessBoard_t board_data, 
                              const ChessBoard_Idx_t *move,
                              const ChessPiece_Tracker_t *piece_tracker) {

  ChessBoard_Idx_t hyp_move[2] = {0};
  const Graph_t *pgraph = piece_tracker->piece_graph;
  const GraphNode_t *const vertices = pgraph->vertices, *moving_vert;
  const GraphEdge_LL_t *edges;
  ChessPiece_Data_t query;
  const ChessPiece_Data_t *moving_pdata, *attacking_pdata, *allied_king_pdata;

  ChessBoard_Idx_t spos = move[0], epos = move[1];
  size_t moving_vert_idx, 
         allied_vert_idx_ofs 
           = (board_data[spos.coord.y][spos.coord.x]&WHITE_FLAGBIT)
                            ? PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT 
                            : 0,
         opp_vert_idx_ofs;
  ChessPiece_e attacking_piece;
  Mvmt_Dir_e attacker_rel_start_loc, start_loc_rel_king, attacker_rel_end_pos;
  opp_vert_idx_ofs = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT^allied_vert_idx_ofs;
  query.location = spos;
  moving_vert = Graph_Get_Vertex(pgraph, &query);
  moving_pdata = moving_vert->data;
  assert(NULL!=moving_pdata);
  moving_vert_idx = moving_vert->idx;

  allied_king_pdata = vertices[KING+allied_vert_idx_ofs].data;
  assert(NULL != allied_king_pdata);
  hyp_move[0] = spos, hyp_move[1] = allied_king_pdata->location;
  start_loc_rel_king = ChessBoard_MoveGetDir(hyp_move);
  if (INVALID_MVMT_FLAGBIT&start_loc_rel_king)
    return FALSE;
  if (!ChessBoard_ValidateMoveClearance(board_data, hyp_move, start_loc_rel_king))
    return FALSE;

  for (size_t i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i) {
    if ((const size_t)i==moving_vert_idx)
      continue;
    edges = &vertices[i+opp_vert_idx_ofs].adj_list;
    attacking_pdata = vertices[i+opp_vert_idx_ofs].data;
    if (NULL==attacking_pdata) {
      assert(0==(piece_tracker->roster.all&(1<<(i+opp_vert_idx_ofs))));
      continue;
    }
    attacking_piece
      = board_data[BOARD_IDX(attacking_pdata->location)]&PIECE_IDX_MASK;
    if (attacking_piece==KNIGHT_IDX || attacking_piece==PAWN_IDX)
      continue;

    LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, cur), cur, edges) {
      if ((const size_t)cur->data.dst_idx != moving_vert_idx)
        continue;
      hyp_move[0] = attacking_pdata->location, hyp_move[1] = spos;
      attacker_rel_start_loc = ChessBoard_MoveGetDir(hyp_move);
      assert(!(attacker_rel_start_loc&INVALID_MVMT_FLAGBIT));
      if (attacker_rel_start_loc&KNIGHT_MVMT_FLAGBIT)
        break;

      if (attacker_rel_start_loc == start_loc_rel_king) {
        if (!ChessBoard_ValidateMoveClearance(board_data, hyp_move, start_loc_rel_king)) {
          // If no clearance, then we know there's another piece behind that 
          // can take the fall for us.
          break;
        }
        if (attacking_pdata->location.raw == epos.raw)
          break;
        hyp_move[0] = attacking_pdata->location, hyp_move[1] = epos;
        attacker_rel_end_pos = ChessBoard_MoveGetDir(hyp_move);
        if (attacker_rel_end_pos==start_loc_rel_king) {
          break;
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}
#ifdef _USE_OLD_KING_ENTERS_CHECK_IMPL_
BOOL ChessBoard_KingMove_EntersCheck(const ChessBoard_t board_data,
                                     const ChessBoard_Idx_t move[2],
                                     const ChessPiece_Tracker_t *tracker) {
  ChessBoard_Idx_t opp_move[2];
  const ChessPiece_e *brow;
  const ChessPiece_Data_t *pdata;
  u32 team_flag, opp_team_flag;
  u32 opp_roster_ofs, dirflag_ct;
  ChessPiece_e piece;
  Mvmt_Dir_e dir;
  ChessPiece_e spiece = board_data[move[0].coord.y][move[0].coord.x];
  team_flag = spiece&PIECE_TEAM_MASK;
  opp_team_flag = PIECE_TEAM_MASK^team_flag;
  if (BLACK_KING!=spiece && WHITE_KING!=spiece)
    return FALSE;
  
  if (team_flag&WHITE_FLAGBIT) {
    opp_roster_ofs = 0;
    if (ROW_8 < move[0].coord.y) {
      brow = board_data[move[0].coord.y-1];
    } else {
      brow = NULL;
    }
  } else {
    opp_roster_ofs = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
    if (ROW_1 > move[0].coord.y) {
      brow = board_data[move[0].coord.y+1];
    } else {
      brow = NULL;
    }
  }
  if (NULL!=brow) {
    ChessPiece_e pawn_attack_pts[2] = {EMPTY_IDX, EMPTY_IDX};
    ChessBoard_File_e file = move[0].coord.x;
    if (file > FILE_A) {
      pawn_attack_pts[0] = brow[file-1];
    }
    if (file < FILE_H) {
      pawn_attack_pts[1] = brow[file+1];
    }
    for (int i = 0; i < 2; ++i) {
      if (pawn_attack_pts[i]==(PAWN_IDX|opp_team_flag))
        return MOVE_UNSUCCESSFUL;
    }
  }
  /* if (brow[FILE_D]==(PAWN_IDX|(PIECE_TEAM_MASK^team_flag)))
   * return MOVE_UNSUCCESSFUL; */
  for (u32 i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i) {
    if (0UL==(tracker->roster.all&(1UL<<(opp_roster_ofs+i))))
      continue;
    pdata = tracker->piece_graph->vertices[i+opp_roster_ofs].data;
    assert(NULL!=pdata);
    opp_move[0].raw = pdata->location.raw;
    opp_move[1] = move[1];
    dir = ChessBoard_MoveGetDir(opp_move);
    dirflag_ct = __builtin_popcount(dir);
    if (INVALID_MVMT_FLAGBIT&dir)
      continue;
    piece = board_data[BOARD_IDX(pdata->location)];
    switch (piece) {
    case BISHOP_IDX:
      if (!(DIAGONAL_MVMT_FLAGBIT&dir))
        continue;
      if (ChessBoard_ValidateMoveClearance(board_data, opp_move, dir))
        return MOVE_UNSUCCESSFUL;
      continue;
    case ROOK_IDX:
      if (1!=__builtin_popcount(dir&(HOR_MASK|VER_MASK)))
        continue;
      if (ChessBoard_ValidateMoveClearance(board_data, opp_move, dir))
        return MOVE_UNSUCCESSFUL;
      continue;
    case KNIGHT_IDX:
      if (!(KNIGHT_MVMT_FLAGBIT&dir))
        continue;
      if (ChessBoard_ValidateMoveClearance(board_data, opp_move, dir))
        return MOVE_UNSUCCESSFUL;
      continue;
    case QUEEN_IDX:
      if (1==dirflag_ct) {
        if (dir!=(dir&(HOR_MASK|VER_MASK)))
          continue;
      } else if (3==dirflag_ct) {
        if (0==(DIAGONAL_MVMT_FLAGBIT&dir) 
            || 1!=__builtin_popcount(dir&HOR_MASK)
            || 1!=__builtin_popcount(dir&VER_MASK))
          continue;
      } else {
        continue;
      }
      if (ChessBoard_ValidateMoveClearance(board_data, opp_move, dir))
        return MOVE_UNSUCCESSFUL;
      continue;
    case KING_IDX:
      if (1==dirflag_ct) {
        if (dir!=(dir&(HOR_MASK|VER_MASK)))
          continue;
      } else if (3==dirflag_ct) {
        if (0==(DIAGONAL_MVMT_FLAGBIT&dir) 
            || 1!=__builtin_popcount(dir&HOR_MASK)
            || 1!=__builtin_popcount(dir&VER_MASK))
          continue;
      } else {
        continue;
      }
      if (1==ABS(opp_move[1].coord.x-opp_move[0].coord.x, 32) 
          && 1==ABS(opp_move[1].coord.y-opp_move[0].coord.y, 32))
        return MOVE_UNSUCCESSFUL;
      continue;
    default:
      continue;
      break;
    }
  }
  return MOVE_UNSUCCESSFUL;
}

#else
BOOL ChessBoard_KingMove_EntersCheck(const ChessBoard_t board_data,
                                     const ChessBoard_Idx_t move[2],
                                     const ChessPiece_Tracker_t *tracker) {
  const GraphNode_t *vertex = tracker->piece_graph->vertices;
  const u32 ROSTER_STATE = tracker->roster.all,
            WHOSE_TURN = PIECE_TEAM_MASK&GET_BOARD_AT_IDX(board_data, move[0]);
  u32 opp_idx_ofs;
  assert(1==__builtin_popcount(WHOSE_TURN));
  opp_idx_ofs = WHOSE_TURN&WHITE_FLAGBIT
                    ? 0 
                    : PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
  vertex+=opp_idx_ofs;
  for (int i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i, ++vertex) {
    assert((int)(i|opp_idx_ofs)==(vertex->idx));
    if (0==(ROSTER_STATE&(1<<(i|opp_idx_ofs)))) {
      assert(NULL==vertex->data);
      continue;
    }
    if (!ChessBoard_ValidateKingMoveEvadesOpp(board_data, move, vertex->data))
      return TRUE;  // If it fails to evade said, opp, then it, by def,
                    // enters check
  }
  return FALSE;
}
#endif
/**
 * @brief returns the number of pieces checking king */
int ChessBoard_KingInCheck(ChessPiece_Data_t **return_pieces,
                            const ChessPiece_Tracker_t *tracker,
                            u32 team) {
  const Graph_t *graph;
  const GraphNode_t *curvert;
  const GraphEdge_LL_Node_t *curedge;
  ChessPiece_Data_t piece_data;
  PieceData_LL_t piece_ll = LL_INIT(PieceData), *pll = &piece_ll;
  ChessPiece_Data_t *return_plist;
  ChessPiece_Roster_t roster;
  u32 opp_idx_ofs, ally_idx_ofs = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
  int ret, i;
  graph=tracker->piece_graph;
  ret = 0;
  opp_idx_ofs = team&WHITE_FLAGBIT?0:PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
  ally_idx_ofs ^= opp_idx_ofs;
  roster = tracker->roster;
  for (u32 i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i) {
    if (!(roster.all&(1UL<<(i+opp_idx_ofs))))
      continue;
    curvert = &graph->vertices[i+opp_idx_ofs];
    for (curedge = curvert->adj_list.head; curedge; curedge = curedge->next) {
      if ((u32)curedge->data.dst_idx!=(ally_idx_ofs|KING))
        continue;

      piece_data = *((const ChessPiece_Data_t*)(curvert->data));
      ++ret;
      LL_NODE_APPEND(PieceData, pll, piece_data);
      break;
    }
  }
  assert((const size_t)ret==piece_ll.nmemb);
  if (0==ret) {
    *return_pieces = NULL;
    return 0;
  }
  return_plist = malloc(sizeof(ChessPiece_Data_t)*ret);
  i = 0;
  for (PieceData_LL_Node_t *curnode=piece_ll.head, *tmp;
       NULL!=curnode;
       curnode=tmp, ++i) {
    tmp = curnode->next;
    return_plist[i] = curnode->data;
    free(curnode);
    --piece_ll.nmemb;
  }
  piece_ll.head = piece_ll.tail = NULL;
  *return_pieces = return_plist;
  assert(0==piece_ll.nmemb);
  assert((const int)ret == i);
  return ret;

}

BOOL ChessBoard_ValidateMoveAddressesCheck(
                                      const ChessGameCtx_t *ctx,
                                      const ChessPiece_Data_t *checking_pieces,
                                      u32 checking_piece_ct,
                                      Move_Validation_Flag_e move_result) {
  ChessBoard_Idx_t auxmove[2];
  const Graph_t *graph;
  const ChessPiece_Data_t *allied_king_data;
  ChessBoard_Idx_t spos, epos, allied_king_pos, tmp, checker_pos;
  ChessPiece_Roster_t roster;
  u32 allied_vert_ofs, turn;
  Mvmt_Dir_e move_rel_king_dir, checker_rel_king_dir, check_rel_move_dir;
  
  assert(0 < checking_piece_ct);
  assert(NULL!=checking_pieces);
  turn = ctx->whose_turn;
  allied_vert_ofs = turn&WHITE_FLAGBIT
                      ? PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT
                      : 0;
  graph = ctx->tracker.piece_graph;
  allied_king_data
    = ((ChessPiece_Data_t*)
      (graph->vertices[allied_vert_ofs|KING].data));
  assert(NULL!=allied_king_data);
  spos = ctx->move_selections[0], 
       epos = ctx->move_selections[1],
       allied_king_pos = allied_king_data->location;
  roster = ctx->tracker.roster;
  if (1==checking_piece_ct) {
    auxmove[0] = epos; 
    auxmove[1] = allied_king_pos;
    move_rel_king_dir = ChessBoard_MoveGetDir(auxmove);
    if (MOVE_CAPTURE&move_result)
      if ((u64)checking_pieces->location.raw == epos.raw)
        return TRUE;
    if (KNIGHT_MVMT_FLAGBIT&move_rel_king_dir)
      return FALSE;
    auxmove[0] = checking_pieces->location;
    checker_rel_king_dir = ChessBoard_MoveGetDir(auxmove);
    if (checker_rel_king_dir!=move_rel_king_dir)
      return FALSE;
    auxmove[0] = epos;
    if (ChessBoard_ValidateMoveClearance(
                              ctx->board_data,
                              auxmove,
                              move_rel_king_dir)) {
      auxmove[0] = checking_pieces->location;
      auxmove[1] = epos;
      check_rel_move_dir = ChessBoard_MoveGetDir(auxmove);
      assert((const Mvmt_Dir_e)checker_rel_king_dir==check_rel_move_dir);
      return ChessBoard_ValidateMoveClearance(
                              ctx->board_data,
                              auxmove,
                              check_rel_move_dir);
    }
    return FALSE;
  }
  if ((turn|KING_IDX)!=ctx->board_data[BOARD_IDX(spos)])
    return FALSE;

  auxmove[1] = epos;
  for (int i = 0; (const int)checking_piece_ct > i; ++i) {
    assert(0!=(roster.all&(1<<(checking_pieces[i].roster_id))));
    checker_pos = auxmove[0] = checking_pieces[i].location;
    if (checker_pos.raw==epos.raw) {
      continue;
    }
    checker_rel_king_dir = ChessBoard_MoveGetDir(auxmove);
    if (INVALID_MVMT_FLAGBIT==checker_rel_king_dir)
      continue;

    switch ((ChessPiece_e)BOARD_BACK_ROWS_INIT[checking_pieces[i].roster_id&PIECE_ROSTER_ID_MASK]) {
    case PAWN_IDX:
      continue;
    case BISHOP_IDX:
      if (!(DIAGONAL_MVMT_FLAGBIT&checker_rel_king_dir))
        continue;
      return FALSE;
    case ROOK_IDX:
      if (1!=__builtin_popcount(checker_rel_king_dir&(VER_MASK|HOR_MASK)))
        continue;
      return FALSE;
    case KNIGHT_IDX:
      if (!(KNIGHT_MVMT_FLAGBIT&checker_rel_king_dir))
        continue;
      return FALSE;
    case QUEEN_IDX:
      if (!(DIAGONAL_MVMT_FLAGBIT&checker_rel_king_dir)) {
        if (1!=__builtin_popcount(checker_rel_king_dir&(VER_MASK|HOR_MASK)))
          continue;
      }
      return FALSE;
    case KING_IDX:
      tmp.coord = (struct s_chess_coord){
        .x=checker_pos.coord.x-epos.coord.x, 
        .y=checker_pos.coord.y-epos.coord.y
      };
      if (1!=ABS(tmp.coord.y,32))
        continue;
      if (1!=ABS(tmp.coord.x,32))
        continue;
      return FALSE;
    default: assert(0==(checking_pieces[i].roster_id&~31)); return FALSE;
    }
  }
  return TRUE;
  
}

Move_Validation_Flag_e ChessBoard_ValidateMove(const ChessGameCtx_t *ctx) {
  static const u64 BOARD_IDX_VALIDITY_MASK=~0x0000000700000007ULL;
  const ChessBoard_Row_t *board = ctx->board_data;
  const ChessBoard_Idx_t *move = ctx->move_selections;
  ChessPiece_e start, end;
  u32 turn = ctx->whose_turn;
  Move_Validation_Flag_e ret;
  if (0ULL != (move[0].raw&BOARD_IDX_VALIDITY_MASK))
    return MOVE_UNSUCCESSFUL;
  if (0ULL != (move[1].raw&BOARD_IDX_VALIDITY_MASK))
    return MOVE_UNSUCCESSFUL;
  { 
    start = board[move[0].coord.y][move[0].coord.x];
    end   = board[move[1].coord.y][move[1].coord.x];
  }

  if (!ChessBoard_ValidateMoveStartPosMatchesTurn(turn, start))
    return MOVE_UNSUCCESSFUL;
  if (!ChessBoard_ValidateMoveEndPosEmptyOrOpp(turn, end))
    return MOVE_UNSUCCESSFUL;
  ret = ChessBoard_ValidateMoveLegality(board, move, &(ctx->tracker), &(ctx->move_hist));
  if (MOVE_UNSUCCESSFUL == ret)
    return MOVE_UNSUCCESSFUL;
  {
    ChessPiece_Data_t *checking_pieces;
    int num_checking_pieces;
    checking_pieces = NULL;
    num_checking_pieces = ChessBoard_KingInCheck(&checking_pieces,
                                                 &(ctx->tracker),
                                                 turn);
    if (0!=num_checking_pieces) {
      assert(NULL!=checking_pieces);
      if (KING_IDX==(start&PIECE_IDX_MASK)) {
        if (!ChessBoard_ValidateKingMoveEvadesCheck(ctx,
                                      checking_pieces,
                                      num_checking_pieces,
                                      ret)) {
          free(checking_pieces);
          checking_pieces = NULL;
          return MOVE_UNSUCCESSFUL;
        }
      } else if (!ChessBoard_ValidateMoveAddressesCheck(ctx,
                                              checking_pieces,
                                              num_checking_pieces,
                                              ret)) {
        free(checking_pieces);
        checking_pieces = NULL;
        return MOVE_UNSUCCESSFUL;
      }
      free(checking_pieces);
      checking_pieces = NULL;
    }
  }

  if (KING_IDX == (start&PIECE_IDX_MASK)) {
    if (ChessBoard_KingMove_EntersCheck(board, move, &(ctx->tracker)))
      return MOVE_UNSUCCESSFUL;
  } else if (ChessBoard_PieceIsPinned(board, move, &(ctx->tracker)))
    return MOVE_UNSUCCESSFUL;
  return ret;
}

int ChessPiece_Data_Cmp_Cb(const void *a, const void *b) {
  ChessPiece_Data_t *da=(ChessPiece_Data_t*)a,
                     *db = (ChessPiece_Data_t*)b;
  int xdiff = da->location.coord.x-db->location.coord.x,
      ydiff = da->location.coord.y-db->location.coord.y;
  if (ydiff)
    return ydiff;
  return xdiff;
  
//  return da->roster_id - db->roster_id;
}


Move_Validation_Flag_e ChessBoard_ValidateEnPassent(const ChessBoard_t board_data,
                                         const ChessBoard_Idx_t move[2],
                                         const PGN_Round_LL_t *mvmt_ll,
                                         ChessPiece_e moving_piece) {
  PGN_Move_t mv;
  if (WHITE_PAWN==moving_piece) {
    mv = mvmt_ll->tail->data.moves[1];
  } else if (BLACK_PAWN==moving_piece) {
    mv = mvmt_ll->tail->data.moves[0];
  } else {
    return MOVE_UNSUCCESSFUL;
  }

#if 0
    if (PAWN_IDX!=BOARD_BACK_ROWS_INIT[mv.roster_id])
      return MOVE_UNSUCCESSFUL;
    if (mv.move[1].coord.y == move[0].coord.y 
        && mv.move[1].coord.x == move[1].coord.x)
    return (ROW_2==mv.move[0].coord.y && ROW_4==mv.move[1].coord.y)
      ? MOVE_SUCCESSFUL|MOVE_CAPTURE|MOVE_EN_PASSENT
      : MOVE_UNSUCCESSFUL;
#else
    if (!(mv.move_outcome&MOVE_PAWN_TWO_SQUARE))
      return MOVE_UNSUCCESSFUL;
    if ((moving_piece|GET_BOARD_AT_IDX(board_data,mv.move[1]))!=PIECE_TEAM_MASK)
      return MOVE_UNSUCCESSFUL;
    ChessBoard_Idx_t loc = {
      .coord = {
        .x = move[1].coord.x, .y = move[0].coord.y
      }
    };
    return mv.move[1].raw != loc.raw
      ? MOVE_UNSUCCESSFUL
      : MOVE_SUCCESSFUL|MOVE_CAPTURE|MOVE_EN_PASSENT;
#endif
  return MOVE_UNSUCCESSFUL;

}




BOOL ChessBoard_ValidateMoveClearance(const ChessBoard_t board_data,
                                      const ChessBoard_Idx_t move[2],
                                      Mvmt_Dir_e dir) {

  // Return true for Knight preemptively because knight doesnt need clearance,
  // and all we care about is whether there is obstruction. Other validity 
  // checks (e.g.: dest is empty or occupied opponent, moving piece is pinned,
  // etc) handled elsewhere.
  if (dir&KNIGHT_MVMT_FLAGBIT)
    return TRUE;
  if (dir&INVALID_MVMT_FLAGBIT)
    return FALSE;
  const int dx, dy;
  switch (dir&VER_MASK) {
  case UP_FLAGBIT:
    *(int*)(&dy) = -1;
    break;
  case DOWN_FLAGBIT:
    *(int*)(&dy) = 1;
    break;
  case 0:
    *(int*)(&dy) = 0;
    break;
  default:
    return FALSE;
  }
  switch (dir&HOR_MASK) {
  case LEFT_FLAGBIT:
    *(int*)(&dx) = -1;
    break;
  case RIGHT_FLAGBIT:
    *(int*)(&dx) = 1;
    break;
  case 0:
    *(int*)(&dx) = 0;
    break;
  default:
    return FALSE;
  }
  if (!dy && !dx)
    return FALSE;
  if (!(dir&DIAGONAL_MVMT_FLAGBIT) && dx && dy)
    return FALSE;
  
  ChessBoard_Idx_t pos = {.raw=move[0].raw};
  for (pos.coord.x+=dx, pos.coord.y+=dy;
       move[1].raw!=pos.raw;
       pos.coord.x+=dx, pos.coord.y+=dy) {
    if ((board_data[pos.coord.y][pos.coord.x]&PIECE_IDX_MASK) != EMPTY_IDX)
      return FALSE;
  }
  return TRUE;
}



Move_Validation_Flag_e ChessBoard_ValidateCastle(
                                            const ChessBoard_Row_t *board_data,
                                            const ChessBoard_Idx_t *move,
                                            const ChessPiece_Tracker_t *tracker,
                                            const PGN_Round_LL_t *mvmt_ll) {
  GraphEdge_LL_t *edges;
  const ChessPiece_e *board_row;
  ChessBoard_Row_e row;
  u32 team_flag, pgn_move_idx, allied_roster_ofs, opp_roster_ofs;
  u16 id;
  team_flag = board_data[move[0].coord.y][move[0].coord.x]&PIECE_TEAM_MASK;

  if (team_flag&WHITE_FLAGBIT) {
    pgn_move_idx = 0;
    row = ROW_1;
    allied_roster_ofs = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
    opp_roster_ofs = 0;
  } else if (team_flag&BLACK_FLAGBIT) {
    pgn_move_idx = 1;
    row = ROW_8;
    allied_roster_ofs = 0;
    opp_roster_ofs = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
  } else {
    assert(0);
    return MOVE_UNSUCCESSFUL;
  }
  
  board_row = board_data[row];
  for (u32 i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i) {
    if (!(tracker->roster.all&(1UL<<(i+opp_roster_ofs))))
      continue;
    edges = &(tracker->piece_graph->vertices[i+opp_roster_ofs].adj_list);
    LL_FOREACH(LL_NODE_VAR_INITIALIZER(GraphEdge, cur), cur, edges) {
      if ((const u32)(cur->data.dst_idx) == KING+allied_roster_ofs)
        return MOVE_UNSUCCESSFUL;
    }
  }

  if (FILE_C == move[1].coord.x) {
    const PGN_Round_LL_Node_t *const LIMIT = WHITE_FLAGBIT&team_flag
                                                      ? NULL
                                                      : mvmt_ll->tail;
    if (board_row[FILE_A]!=(ROOK_IDX|team_flag))
      return MOVE_UNSUCCESSFUL;
    for (ChessBoard_File_e fl = FILE_B; fl < FILE_E; ++fl) {
      if (EMPTY_IDX!=(board_row[fl]&PIECE_IDX_MASK))
        return MOVE_UNSUCCESSFUL;
    }
    for (const PGN_Round_LL_Node_t *cur = mvmt_ll->head;
         LIMIT!=cur;
         cur = cur->next) {
      id = cur->data.moves[pgn_move_idx].roster_id;
      if (ROOK0 == id || KING == id) {
        return MOVE_UNSUCCESSFUL;
      }
    }
    return MOVE_SUCCESSFUL|MOVE_CASTLE_QUEENSIDE;
  } else if (FILE_G == move[1].coord.x) {
    if (board_row[FILE_H]!=(ROOK_IDX|team_flag))
      return MOVE_UNSUCCESSFUL;
    for (ChessBoard_File_e fl = FILE_F; fl < FILE_H; ++fl) {
      if (EMPTY_IDX !=(board_row[fl]&PIECE_IDX_MASK))
        return MOVE_UNSUCCESSFUL;
    }
    for (const PGN_Round_LL_Node_t *cur = mvmt_ll->head; cur; cur = cur->next) {
      id  = cur->data.moves[pgn_move_idx].roster_id;
      if (ROOK1 == id || KING == id) {
        return MOVE_UNSUCCESSFUL;
      }
    }
    return MOVE_SUCCESSFUL|MOVE_CASTLE_KINGSIDE;
  } else {
    return MOVE_UNSUCCESSFUL;
  }

}

#define SINGLE_SQUARE_ATTACKER_MASK       0x0003
#define PAWN_ATTACKER                     0x0001
#define KING_ATTACKER                     0x0002
#define ANY_DIAGONAL_SQUARE_ATTACKER_MASK 0x000C
#define BISHOP_ATTACKER                   0x0004
#define QUEEN_ATTACKER                    0x0008
#define ANY_VER_HOR_SQUARE_ATTACKER_MASK  0x0018
#define ROOK_ATTACKER                     0x0010

BOOL ChessBoard_ValidateKingMoveEvadesOpp(
                                    const ChessBoard_t board_data,
                                    const ChessBoard_Idx_t *move,
                                    const ChessPiece_Data_t *checking_piece) {
  ChessBoard_Idx_t auxmove[2] = {
    checking_piece->location,  // attacker location
    move[1]  // king location
  };
  int xdiff, ydiff;
  ChessPiece_e attacking_piece = GET_BOARD_AT_IDX(board_data, auxmove[0]);
  Mvmt_Dir_e attack_dir, move_dir;

  attack_dir = ChessBoard_MoveGetDir(auxmove);
  move_dir = ChessBoard_MoveGetDir(move);
  if (INVALID_MVMT_FLAGBIT==attack_dir)
    return TRUE;
  
  xdiff = auxmove[0].coord.x-auxmove[1].coord.x;
  ydiff = auxmove[0].coord.y-auxmove[1].coord.y;
  switch (PIECE_IDX_MASK&attacking_piece) {
  case PAWN_IDX:
    if (1!=ABS(xdiff, 32))
      return TRUE;
    if (WHITE_PAWN==attacking_piece) {
      if (1!=ydiff)
        return TRUE;
    } else if (-1!=ydiff) {
      return TRUE;
    }
    break;
  case BISHOP_IDX:
    if (!(DIAGONAL_MVMT_FLAGBIT&attack_dir))
      return TRUE;
  case QUEEN_IDX:
    if (KNIGHT_MVMT_FLAGBIT&attack_dir)
      return TRUE;
    if (move_dir==attack_dir) {
      return FALSE;
    }
    if (!ChessBoard_ValidateMoveClearance(board_data, auxmove, attack_dir))
      return TRUE;
    break;
  case KING_IDX:
    if (1!=ABS(ydiff,32) || 1!=ABS(xdiff,32))
      return TRUE;
    break;
  case ROOK_IDX:
    if ((DIAGONAL_MVMT_FLAGBIT|KNIGHT_MVMT_FLAGBIT)&attack_dir)
      return TRUE;
    assert(1==__builtin_popcount(attack_dir&(HOR_MASK|VER_MASK)));
    if (move_dir==attack_dir)
      return FALSE;
    if (!ChessBoard_ValidateMoveClearance(board_data, auxmove, attack_dir))
      return TRUE;
    break;
  case KNIGHT_IDX:
    if (!(KNIGHT_MVMT_FLAGBIT&attack_dir))
      return TRUE;
    break;
  default: 
    assert(EMPTY_IDX!=(PIECE_IDX_MASK&attacking_piece));
    assert(0);
  }
  return FALSE;
  

  
}


                                              

BOOL ChessBoard_ValidateKingMoveEvadesCheck(
                                     const ChessGameCtx_t *ctx,
                                     const ChessPiece_Data_t *checking_pieces,
                                     u32 checking_piece_ct,
                                     Move_Validation_Flag_e move_result) {
  const u32 ROSTER_STATE = ctx->tracker.roster.all;
  for (u32 i = 0; (const u32)checking_piece_ct > i; ++i) {
    assert(0!=(ROSTER_STATE&(1<<(checking_pieces[i].roster_id))));
    if (move_result&MOVE_CAPTURE) {
      if (checking_pieces[i].location.raw==ctx->move_selections[1].raw)
        continue;
    }
    if (ChessBoard_ValidateKingMoveEvadesOpp(ctx->board_data,
                                             ctx->move_selections,
                                             &checking_pieces[i]))
      continue;
    return FALSE;
  }
  return TRUE;

}

/**
 * @brief returns false if it hits a wall, and will update arg3's ptr to
 * show what wall square it reached, otherwise, if hits another piece,
 * it'll return true, and give the piece idx in arg3 out/return ptr.
 **/
bool ChessBoard_FindNextObstruction(const ChessBoard_t board_data,
                                    const ChessBoard_Idx_t *start,
                                    ChessBoard_Idx_t *return_obstruction_idx,
                                    Mvmt_Dir_e dir) {

  ChessBoard_Idx_t idx = {.raw = start->raw}, last_valid_idx;
  BOOL ret = FALSE;
  *return_obstruction_idx = *start;
  
  int dy, dx;
  if (0!=(INVALID_MVMT_FLAGBIT&dir) || 0!=(KNIGHT_MVMT_FLAGBIT&dir)) {
    return_obstruction_idx->raw = 0xFFFFFFFFFFFFFFFFULL;
    return FALSE;
  }

  switch (VER_MASK&dir) {
  case UP_FLAGBIT:
    dy = -1;
    break;
  case DOWN_FLAGBIT: 
    dy = 1;
    break;
  case 0:
    dy = 0;
    break;
  default:
    assert(HOR_MASK!=(HOR_MASK&dir));
    break;
  }
  switch (HOR_MASK&dir) {
  case LEFT_FLAGBIT: 
    dx = -1;
    break;
  case RIGHT_FLAGBIT: 
    dx = 1;
    break;
  case 0:
    dx = 0;
    break;
  default:
    assert(VER_MASK!=(VER_MASK&dir));
    break;
  }
  
  if (dy && dx) {
    assert(DIAGONAL_MVMT_FLAGBIT&dir);
  }
  assert(dy || dx);
  last_valid_idx = idx;
  for (idx.coord.x+=dx, idx.coord.y+=dy;
       0ULL==(idx.raw&0xFFFFFFF8FFFFFFF8ULL); 
       idx.coord.x+=dx, idx.coord.y+=dy) {
    last_valid_idx = idx;
    if (EMPTY_IDX!=(PIECE_IDX_MASK&GET_BOARD_AT_IDX(board_data, idx))) {
      ret = TRUE;
      break;
    }
  }
  *return_obstruction_idx = last_valid_idx;
  return ret;
}

Knight_Mvmt_Dir_e ChessBoard_KnightMoveGetDir(const ChessBoard_Idx_t *mv,
                                              Mvmt_Dir_e dir) {
  int abs_dx, abs_dy;
  Knight_Mvmt_Dir_e ret = dir&(HOR_MASK|VER_MASK);
  if (!(dir&KNIGHT_MVMT_FLAGBIT))
    return 0;
  abs_dx = mv[1].coord.x - mv[0].coord.x;
  abs_dy = mv[1].coord.y - mv[0].coord.y;
  abs_dx = ABS(abs_dx, 32);
  abs_dy = ABS(abs_dy, 32);
  if (0!=(abs_dy&~3)) {
    return 0;
  }
  if (0!=(abs_dx&~3)) {
    return 0;
  }
  if (3!=(abs_dx^abs_dy)) {
    return 0;
  }
  if (abs_dx==2)
    ret|=KNIGHT_MVMT_WIDE_FLAGBIT;
  else if (abs_dy==2)
    ret|=KNIGHT_MVMT_TALL_FLAGBIT;
  else
    return 0;
  return ret;
}
