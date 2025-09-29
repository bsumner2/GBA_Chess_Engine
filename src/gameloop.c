#include <GBAdev_types.h>
#include <GBAdev_functions.h>
#include <GBAdev_memdef.h>
#include <GBAdev_memmap.h>
#include <GBAdev_util_macros.h>
#include <iso646.h>
#include <stdlib.h>
#include "graph.h"
#include "key_status.h"
#include "chess_board.h"
#include "chess_obj_sprites_data.h"
#include "chess_sprites.h"
#include "linked_list.h"

#define UPDATE_ALL
extern EWRAM_CODE void Vsync(void);

/*static __attribute__((__naked__)) void IRQ_Sync(u32 flags) {
   ASM (
        "\tSVC 0x04\n"
        "\tBX lr\n"
      :::"r0","r1","r2","r3");
}*/
extern void IRQ_Sync(u32 flags);


static void ChessGame_UpdateMoveHistory(ChessGameCtx_t *ctx,
                                        Move_Validation_Flag_e move_outcome);
static void ChessGame_UpdateBoardAndGraph(ChessGameCtx_t *ctx, 
                                  Move_Validation_Flag_e move);
#ifdef UPDATE_ALL
static void ChessGame_UpdateVertexEdges(const ChessBoard_t board_data, 
                                        Graph_t *pgraph,
                                        GraphNode_t *moving_vert);
#else
static void ChessGame_UpdateEdges(const ChessBoard_t board_data,
                                  Graph_t *graph,
                                  const ChessBoard_Idx_t *last_move,
                                  GraphNode_t *node);
#endif
#ifndef UPDATE_ALL
static void ChessGame_UpdateMovedPieceEdges(const ChessBoard_t board_data,
                                            Graph_t *pgraph,
                                            const ChessBoard_Idx_t move[2],
                                            GraphNode_t *moving_vert);
#endif
static void ChessGame_PromotionPrompt(ChessGameCtx_t *ctx, int idx);
INLN void UPDATE_PIECE_SPRITE_LOCATION(Obj_Attr_t *spr_obj, ChessBoard_Idx_t move);

#define PROMOTION_SEL_MASK 3
static const ChessPiece_e PROMOTION_SEL[] = {
  QUEEN_IDX, ROOK_IDX, BISHOP_IDX, KNIGHT_IDX
};

void ChessGame_PromotionPrompt(ChessGameCtx_t *ctx, int idx) {
  const u32 spr_ofs = ctx->whose_turn&WHITE_FLAGBIT
                          ?0:2*TILES_PER_CSPR*Chess_sprites_Glyph_Count;
  ChessBoard_Idx_t *move = ctx->move_selections;
  int promotion_id=0;
  Obj_Attr_t tmp = ctx->obj_data.sels[1];
  tmp.attr0.regular.mode = OBJ_GFX_MODE_NORMAL;
  tmp.attr2 = (Obj_Attr_Fields2_t){
    .sprite_idx = PROMOTION_SEL[promotion_id]*2*CSPR_TILES_PER_DIM+spr_ofs,
    .priority = 0
  };
  OAM_Copy(&OAM_ATTR[idx+2], &tmp, 1);
  for (BOOL sel = FALSE, sprchange=TRUE; !sel; 
       sel=(0!=KEY_STROKE(A)), sprchange=TRUE) {
    IRQ_Sync(1<<IRQ_KEYPAD);
    if (KEY_STROKE(LEFT)) {
      --promotion_id;
      promotion_id&=PROMOTION_SEL_MASK;
    } else if (KEY_STROKE(RIGHT)) {
      ++promotion_id;
      promotion_id&=PROMOTION_SEL_MASK;
    } else {
      sprchange = FALSE;
    }
    if (!sprchange) continue;
    tmp.attr2.sprite_idx = spr_ofs+PROMOTION_SEL[promotion_id]*2*TILES_PER_CSPR;
    OAM_Copy(&OAM_ATTR[idx+2], &tmp, 1);
  }
  tmp.attr0.regular.disable=TRUE;
  OAM_Copy(&OAM_ATTR[idx+2], &tmp, 1);

  ctx->board_data[BOARD_IDX(move[0])] 
                    = (ctx->whose_turn|PROMOTION_SEL[promotion_id]);
}

Move_Validation_Flag_e ChessGame_GetMove(ChessGameCtx_t *ctx) {
  Obj_Attr_t *sels = ctx->obj_data.sels;
  ChessBoard_Idx_t curmove={0}, *move = ctx->move_selections;
  u32 idx = ((UIPTR_T)(sels) - (UIPTR_T)&(ctx->obj_data))/sizeof(Obj_Attr_t);

  sels[0].attr0 = (Obj_Attr_Fields0_t) {
    .regular = {
      .y = CHESS_BOARD_Y_OFS,
      .mode = OBJ_GFX_MODE_BLEND,
      .pal_8bpp = TRUE,
    }
  };

  if (ctx->whose_turn&WHITE_FLAGBIT) {
    sels[0].attr0.regular.y += Chess_sprites_Glyph_Height*(CHESS_BOARD_ROW_COUNT-1);
    curmove.coord.y = ROW_1;
  }
  sels[0].attr1.regular.x = CHESS_BOARD_X_OFS;
  sels[0].attr2.sprite_idx = EMPTY_IDX*TILES_PER_CSPR*2;
  sels[1] = sels[0];
  for (int i=0; 2>i; ++i, Vsync()) {
    OAM_Copy(&OAM_ATTR[idx+i], &sels[i], 1);
    for (BOOL sel=FALSE; !sel; sel=0!=KEY_STROKE(A),
                                      OAM_Copy(&OAM_ATTR[idx+i], &sels[i], 1),
                                      Vsync()) {
      IRQ_Sync(1<<IRQ_KEYPAD);
      if (KEY_STROKE(UP)) {
        if (ROW_8 < curmove.coord.y) {
          --curmove.coord.y;
          sels[i].attr0.regular.y-=Chess_sprites_Glyph_Height;
        }
      } else if (KEY_STROKE(DOWN)) {
        if (ROW_1 > curmove.coord.y) {
          ++curmove.coord.y;
          sels[i].attr0.regular.y+= Chess_sprites_Glyph_Height;
        }
      }
      if (KEY_STROKE(LEFT)) {
        if (FILE_A < curmove.coord.x) {
          --curmove.coord.x;
          sels[i].attr1.regular.x -= Chess_sprites_Glyph_Width;
        }
      } else if (KEY_STROKE(RIGHT)) {
        if (FILE_H > curmove.coord.x) {
          ++curmove.coord.x;
          sels[i].attr1.regular.x += Chess_sprites_Glyph_Width;
        }
      }
    }
    move[i] = curmove;
    if (i)
      continue;
    if (ctx->whose_turn&WHITE_FLAGBIT) {
      if (ROW_8 < curmove.coord.y)
        --curmove.coord.y;
      else if (FILE_A < curmove.coord.x)
        --curmove.coord.x;
      else
        ++curmove.coord.x;
    } else {
      if (ROW_1 > curmove.coord.y)
        ++curmove.coord.y;
      else if (FILE_A < curmove.coord.x)
        --curmove.coord.x;
      else
        ++curmove.coord.x;
    }
    UPDATE_PIECE_SPRITE_LOCATION(&sels[i+1], curmove);
  }
  if (PAWN_IDX==(ctx->board_data[BOARD_IDX(move[0])]&PIECE_IDX_MASK)) {
    if (ctx->whose_turn&WHITE_FLAGBIT) {
      if (ctx->move_selections[1].coord.y==ROW_8)
        ChessGame_PromotionPrompt(ctx, idx);
    } else if (ctx->move_selections[1].coord.y==ROW_1) {
      ChessGame_PromotionPrompt(ctx, idx);
    }
  }

  sels[0].attr0.regular.disable = TRUE;
  sels[1].attr0.regular.disable = TRUE;
  OAM_Copy(&OAM_ATTR[idx], sels, 2);
  return ChessBoard_ValidateMove(ctx);
}

void ChessGame_UpdateMoveHistory(ChessGameCtx_t *ctx,
                                 Move_Validation_Flag_e move_outcome) {
  PGN_Round_t round = {0};
  ChessPiece_Data_t query = {0};
  PGN_Round_LL_t *rll = &ctx->move_hist;
  GraphNode_t *mvert;
  query.location = ctx->move_selections[1];
  mvert = Graph_Get_Vertex(ctx->tracker.piece_graph, &query);
  assert(NULL!=mvert);
  assert(NULL!=mvert->data);
  if (ctx->whose_turn&WHITE_FLAGBIT) {
    round.moves[0] = (PGN_Move_t){
      .roster_id = (mvert->idx)&PIECE_ROSTER_ID_MASK,
      .move = {
        ctx->move_selections[0],
        ctx->move_selections[1]
      },
      .move_outcome = move_outcome
    };
    LL_NODE_APPEND(PGN_Round, rll, round);
  } else {
    assert(NULL!=rll->tail);
    rll->tail->data.moves[1] = (PGN_Move_t){
      .roster_id = (mvert->idx)&PIECE_ROSTER_ID_MASK,
      .move = {
        ctx->move_selections[0],
        ctx->move_selections[1]
      },
      .move_outcome = move_outcome
    };
  }
}

int ObjAttrCmp(const void *a, const void *b) {
  Obj_Attr_t *oa=*(Obj_Attr_t**)a, *ob=*(Obj_Attr_t**)b;
  return oa->attr2.sprite_idx-ob->attr2.sprite_idx;
}

void ChessBoard_DrawCapturedTeam(const Obj_Attr_t *obj_origin,
                                 ChessPiece_Tracker_t *tracker, 
                                 u32 captured_team) {
  Obj_Attr_t **objs, *cur;
  u32 x,y, dx, dy;
  int count, col_row_ct = 0, ofs;
  if (captured_team&WHITE_FLAGBIT) {
    objs = tracker->pieces_captured[1];
    count = tracker->capcount[1];
    x = SCREEN_WIDTH - CHESS_BOARD_X_OFS/2, 
      dx = -Chess_sprites_Glyph_Width;
    y = CHESS_BOARD_Y_OFS, 
      dy = Chess_sprites_Glyph_Height;
  } else {
    objs = tracker->pieces_captured[0];
    count = tracker->capcount[0];
    x = CHESS_BOARD_X_OFS/2 - Chess_sprites_Glyph_Width, 
      dx = Chess_sprites_Glyph_Width;
    y = SCREEN_HEIGHT - CHESS_BOARD_Y_OFS - Chess_sprites_Glyph_Height,
      dy = -Chess_sprites_Glyph_Height;
  }
  const u32 Y_ORIGIN = y;
  for (int i = 0; i < count; ++i, ++col_row_ct, y+=dy) {
    cur = objs[i];
    ofs = ((UIPTR_T)cur-(UIPTR_T)obj_origin)/sizeof(Obj_Attr_t);
    if (8 <= col_row_ct) {
      col_row_ct = 0;
      y=Y_ORIGIN;
      x+=dx;
    }
    cur->attr0.regular.y = y;
    cur->attr1.regular.x = x;
    OAM_Copy(&OAM_ATTR[ofs], cur, 1);
  }
}

#ifndef UPDATE_ALL
void ChessGame_UpdateEdges(const ChessBoard_t board_data,
                           Graph_t *graph,
                           const ChessBoard_Idx_t *last_move,
                           GraphNode_t *node) {
  ChessPiece_Data_t piece_data, query;
  ChessBoard_Idx_t mv[2];
  ChessPiece_Data_t *cur_opp_data;
  GraphNode_t *opp_vertex, *moving_vert;
  u32 opp_flag, ally_flag;
  ChessPiece_e piece_type;
  Mvmt_Dir_e attacker_rel_edge_pertinent, attacker_rel_last_moved;
  BOOL potentially_impactful;
  piece_data = *(ChessPiece_Data_t*)node->data;
  piece_type = GET_BOARD_AT_IDX(board_data, piece_data.location);
  ally_flag = (piece_type&PIECE_TEAM_MASK);
  opp_flag = ally_flag^PIECE_TEAM_MASK;
  assert(0!=opp_flag);
  mv[0] = piece_data.location;
  mv[1] = last_move[1];
  attacker_rel_last_moved = ChessBoard_MoveGetDir(mv);
  query = (ChessPiece_Data_t){.location=mv[1], .obj_ctl=NULL, .roster_id=0};
  moving_vert = Graph_Get_Vertex(graph, &query);
  assert(NULL!=moving_vert);
  assert(NULL!=moving_vert->data);
  if (INVALID_MVMT_FLAGBIT) {
    potentially_impactful = FALSE;
  } else {
    switch ((piece_type&PIECE_IDX_MASK)) {
    case PAWN_IDX:
      if (DIAGONAL_MVMT_FLAGBIT&attacker_rel_last_moved) {
        int dy=mv[1].coord.y-mv[0].coord.y,dx=mv[1].coord.x-mv[0].coord.x;
        if (ally_flag&WHITE_FLAGBIT) {
          if (-1==dy && 1==ABS(dx, 32))
            potentially_impactful = TRUE;
          
        } else if (1==dy && 1==ABS(dx,32)) {
          potentially_impactful = TRUE;
        }

        if (potentially_impactful) {
          if (opp_flag&GET_BOARD_AT_IDX(board_data, mv[1])) {
            assert(Graph_Add_Edge(graph, node->idx, moving_vert->idx, 1));
            potentially_impactful = FALSE;
          }
        }
      }
      break;
    case BISHOP_IDX:
      if (DIAGONAL_MVMT_FLAGBIT&attacker_rel_last_moved)
        potentially_impactful = TRUE;
      break;
    case ROOK_IDX:
      if (1==__builtin_popcount(attacker_rel_last_moved&(HOR_MASK|VER_MASK)))
        potentially_impactful=TRUE;
      break;
    case KNIGHT_IDX:
      if (KNIGHT_MVMT_FLAGBIT&attacker_rel_last_moved) {
        if (opp_flag&GET_BOARD_AT_IDX(board_data, mv[1])) {
          assert(Graph_Add_Edge(graph, node->idx, moving_vert->idx, 1));
          potentially_impactful = FALSE;
        }
      }
    case QUEEN_IDX:
      if (KNIGHT_MVMT_FLAGBIT&attacker_rel_last_moved)
        break;
      potentially_impactful = TRUE;
      break;
    case KING_IDX:
      if (!(KNIGHT_MVMT_FLAGBIT&attacker_rel_last_moved)) {
        int dy=mv[1].coord.y-mv[0].coord.y,dx=mv[1].coord.x-mv[0].coord.x;
        if (1==ABS(dy,32) && 1==ABS(dx,32)) {
          if (opp_flag&GET_BOARD_AT_IDX(board_data, mv[1])) {
            assert(Graph_Add_Edge(graph, node->idx, moving_vert->idx, 1));
          }
        }
      }
      break;
    default: assert(0);
    }
  }
  if (!potentially_impactful)
    return;
  for (GraphEdge_LL_Node_t *cur=node->adj_list.head; cur; cur=cur->next) {
    cur_opp_data = (ChessPiece_Data_t*)graph->vertices[cur->data.dst_idx].data;
    assert(NULL!=cur_opp_data);
    mv[1] = cur_opp_data->location;
    attacker_rel_edge_pertinent = ChessBoard_MoveGetDir(mv);
    assert(INVALID_MVMT_FLAGBIT!=attacker_rel_edge_pertinent);
    if (attacker_rel_last_moved==attacker_rel_edge_pertinent) {

      
    }
  }
}
#endif

/**
 * @brief returns false if it hits a wall, and will update arg3's ptr to
 * show what wall square it reached, otherwise, if hits another piece,
 * it'll return true, and give the piece idx in arg3 out/return ptr.
 **/
bool ChessBoard_FindNextObstruction(const ChessBoard_t board_data,
                                    const ChessBoard_Idx_t *start,
                                    ChessBoard_Idx_t *return_obstruction_idx,
                                    Mvmt_Dir_e dir) {

  ChessBoard_Idx_t idx = {.raw = start->raw};
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
  for (idx.coord.x+=dx, idx.coord.y+=dy;
       0ULL==(idx.raw&0xFFFFFFF8FFFFFFF8ULL); 
       idx.coord.x+=dx, idx.coord.y+=dy) {
    if (EMPTY_IDX!=(PIECE_IDX_MASK&GET_BOARD_AT_IDX(board_data, idx))) {
      ret = TRUE;
      break;
    }
  }
  *return_obstruction_idx = idx;
  return ret;
}
#ifdef UPDATE_ALL
void ChessGame_UpdateVertexEdges(const ChessBoard_t board_data,
                           Graph_t *pgraph,
                           GraphNode_t *moving_vert) {
  Mvmt_Dir_e mvmts[8] = {
    INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, 
    INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, 
    INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT
  };
  ChessPiece_Data_t query = {0};
  ChessBoard_Idx_t origin;
  ChessPiece_Data_t *pdata = (ChessPiece_Data_t*)moving_vert->data;
  GraphNode_t *new_hit;
  u32 ally_flag,
      opp_flag;

  ChessPiece_e new_hit_piece, mpiece;
  origin = pdata->location;
  mpiece = GET_BOARD_AT_IDX(board_data, origin);
  ally_flag = mpiece&PIECE_TEAM_MASK,
  opp_flag  = ally_flag^PIECE_TEAM_MASK;
  int ct = 0;
  switch (mpiece&PIECE_IDX_MASK) {
  case PAWN_IDX:
    {
      ChessBoard_Idx_t tmp[2] = {origin, origin};
      if (ally_flag&WHITE_FLAGBIT) {
        assert(ROW_8<origin.coord.y);
        --tmp[0].coord.y;
        --tmp[1].coord.y;
      } else {
        assert(ROW_1>origin.coord.y);
        ++tmp[0].coord.y;
        ++tmp[1].coord.y;
      }
      if (FILE_A < origin.coord.x) {
        --tmp[0].coord.x;
      }
      if (FILE_H > origin.coord.x) {
        ++tmp[1].coord.x;
      }
      for (int i = 0; i < 2; ++i) {
        if (tmp[i].coord.x == origin.coord.x)
          continue;
        new_hit_piece = GET_BOARD_AT_IDX(board_data, tmp[i]);
        if (!(new_hit_piece&opp_flag))
          continue;
        query.location = tmp[i];
        new_hit = Graph_Get_Vertex(pgraph, &query);
        assert(NULL!=new_hit);
        assert(NULL!=new_hit->data);
        assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
      }
    }
    ct = 0;
    break;
  case BISHOP_IDX: 
    if (FILE_A < origin.coord.x) {
      if (ROW_8 < origin.coord.y) {
        mvmts[0] = UP_FLAGBIT|LEFT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
      }
      if (ROW_1 > origin.coord.y) {
        mvmts[1] = DOWN_FLAGBIT|LEFT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
      }
    }
    if (FILE_H > origin.coord.x) {
      if (ROW_8 < origin.coord.y) {
        mvmts[2] = UP_FLAGBIT|RIGHT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
      }
      if (ROW_1 > origin.coord.y) {
        mvmts[3] = DOWN_FLAGBIT|RIGHT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
      }
    }
    ct = 4;
    break;
  case ROOK_IDX:
    {
      if (FILE_A < origin.coord.x) {
        mvmts[0] = LEFT_FLAGBIT;
      }
      if (FILE_H > origin.coord.x) {
        mvmts[1] = RIGHT_FLAGBIT;
      }
      if (ROW_8 < origin.coord.y) {
        mvmts[2] = UP_FLAGBIT;
      }
      if (ROW_1 > origin.coord.y) {
        mvmts[3] = DOWN_FLAGBIT;
      }
    }
    ct = 4;
    break;
  case KNIGHT_IDX:
    {
      ChessBoard_Idx_t idxs[8];
      bool valid[8] = {0};
      if (FILE_A < origin.coord.x) {
        if (ROW_7 < origin.coord.y) {
          idxs[0].coord = (struct s_chess_coord){
            .x = origin.coord.x-1,
            .y = origin.coord.y-2
          };
          valid[0] = TRUE;
        }
        if (FILE_B < origin.coord.x) {
          if (ROW_8 < origin.coord.y) {
            idxs[1].coord = (struct s_chess_coord){
              .x=origin.coord.x-2,
              .y=origin.coord.y-1
            };
            valid[1] = TRUE;
          }
          if (ROW_1 > origin.coord.y) {
            idxs[2].coord = (struct s_chess_coord){
              .x = origin.coord.x-2,
              .y = origin.coord.y+1
            };
            valid[2] = TRUE;
          }
        }
        if (ROW_2 > origin.coord.y) {
          idxs[3].coord = (struct s_chess_coord){
            .x = origin.coord.x-1,
            .y = origin.coord.y+2
          };
          valid[3] = TRUE;
        }
      }
      if (FILE_H > origin.coord.x) {
        if (ROW_7 < origin.coord.y) {
          idxs[4].coord = (struct s_chess_coord){
            .x = origin.coord.x+1,
            .y = origin.coord.y-2
          };
          valid[4] = TRUE;
        }
        if (FILE_G > origin.coord.x) {
          if (ROW_8 < origin.coord.y) {
            idxs[5].coord = (struct s_chess_coord){
              .x = origin.coord.x+2,
              .y = origin.coord.y-1
            };
            valid[5] = TRUE;
          }
          if (ROW_1 > origin.coord.y) {
            idxs[6].coord = (struct s_chess_coord){
              .x = origin.coord.x+2,
              .y = origin.coord.y+1
            };
            valid[6] = TRUE;
          }
        }
        if (ROW_2 > origin.coord.y) {
          idxs[7].coord = (struct s_chess_coord){
            .x = origin.coord.x+1,
            .y = origin.coord.y+2
          };
          valid[7] = TRUE;
        }
      }
      for (int i = 0; i < 8; ++i) {
        if (!valid[i])
          continue;
        new_hit_piece = GET_BOARD_AT_IDX(board_data, idxs[i]);
        if (!(new_hit_piece&opp_flag))
          continue;
        query.location = idxs[i];
        new_hit = Graph_Get_Vertex(pgraph, &query);
        assert(NULL!=new_hit);
        assert(NULL!=new_hit->data);
        assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
      }
    }
    ct = 0;
    break;
  case QUEEN_IDX:
    if (FILE_A < origin.coord.x) {
      mvmts[0] = LEFT_FLAGBIT;
      mvmts[1] |= LEFT_FLAGBIT;
      mvmts[2] |= LEFT_FLAGBIT;
    }
    if (FILE_H > origin.coord.x) {
      mvmts[3] = RIGHT_FLAGBIT;
      mvmts[4] |= RIGHT_FLAGBIT;
      mvmts[5] |= RIGHT_FLAGBIT;
    }
    if (ROW_8 < origin.coord.y) {
      mvmts[6] = UP_FLAGBIT;
      mvmts[1] |= UP_FLAGBIT;
      mvmts[4] |= UP_FLAGBIT;
    }
    if (ROW_1 > origin.coord.y) {
      mvmts[7] = DOWN_FLAGBIT;
      mvmts[2] |= DOWN_FLAGBIT;
      mvmts[5] |= DOWN_FLAGBIT;
    }
    {
      Mvmt_Dir_e curmv;
      for (int i = 0, j; 6 > i; i+=3) {
        for (j = 1; 3 > j; ++j) {
          curmv = mvmts[i+j];
          if ((curmv&HOR_MASK) && (curmv&VER_MASK)) {
            mvmts[i+j]^=(DIAGONAL_MVMT_FLAGBIT|INVALID_MVMT_FLAGBIT);
          }
        }
      }
    }
    ct = 8;
    break;
  case KING_IDX:
    {
      ChessBoard_Idx_t idx = origin;
      int i, j, di;
      for (j = -1; 2 > j; ++j, idx.coord.y=origin.coord.y) {
        idx.coord.y += j;

        if (0UL!=(idx.coord.y&~7UL))
          continue;
        di = 0==j ? 2 : 1;
        for (i=-1; 2 > i; i+=di, idx.coord.x=origin.coord.x) {
          idx.coord.x+=i;
          if (0UL!=(idx.coord.x&~7UL))
            continue;
          new_hit_piece = GET_BOARD_AT_IDX(board_data, idx);
          if (!(new_hit_piece&opp_flag))
            continue;
          query.location = idx;
          new_hit = Graph_Get_Vertex(pgraph, &query);
          assert(NULL!=new_hit);
          assert(NULL!=new_hit->data);
          assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
        }
      }
    }
    ct = 0;
    break;
  default: assert(0);
  }
  if (0!=ct) {
    Mvmt_Dir_e curmv;
    for (int i = 0; i < ct; ++i) {
      curmv = mvmts[i];
      if (INVALID_MVMT_FLAGBIT==curmv)
        continue;
      if (!ChessBoard_FindNextObstruction(board_data, &origin, &query.location, curmv))
        continue;
      new_hit_piece = GET_BOARD_AT_IDX(board_data, query.location);
      if (!(new_hit_piece&opp_flag))
        continue;
      new_hit = Graph_Get_Vertex(pgraph, &query);
      assert(NULL!=new_hit);
      assert(NULL!=new_hit->data);
      assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
    }
  }
}
#else
void ChessGame_UpdateMovedPieceEdges(const ChessBoard_t board_data,
                                     Graph_t *pgraph,
                                     const ChessBoard_Idx_t move[2],
                                     GraphNode_t *moving_vert) {
  ChessBoard_Idx_t mv[2];

  ChessPiece_Data_t query={0};
  ChessPiece_Data_t *pdata, *mdata;
  GraphNode_t *curnode, *new_hit;
  u32 ally_flag, ally_ofs = moving_vert->idx&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT, 
      opp_flag, opp_ofs = ally_ofs^PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT;
  ChessPiece_e mpiece, new_hit_piece;
  Mvmt_Dir_e mvdir;
  mdata = moving_vert->data;
  mv[1] = move[0];
  mpiece = GET_BOARD_AT_IDX(board_data, move[1]);
  ally_flag = PIECE_TEAM_MASK&mpiece;
  assert(0!=ally_flag);
  opp_flag = PIECE_TEAM_MASK^mpiece;
  assert(0!=opp_flag);
  for (int i = 0; i < CHESS_TEAM_PIECE_COUNT; ++i, mv[1]=move[0]) {
    curnode = &pgraph->vertices[i|opp_ofs];
    pdata = curnode->data;
    if (NULL==curnode->data)
      continue;
    if (!Graph_Has_Edge(pgraph, curnode->idx, moving_vert->idx))
      continue;
    Graph_Remove_Edge(pgraph, curnode->idx, moving_vert->idx);
    mv[0] = pdata->location;
    mvdir = ChessBoard_MoveGetDir(mv);
    assert(INVALID_MVMT_FLAGBIT!=mvdir);
    if (KNIGHT_MVMT_FLAGBIT&mvdir) {
      assert(KNIGHT_IDX==(PIECE_IDX_MASK&GET_BOARD_AT_IDX(board_data, mv[0])));
      continue;
    }
    if (!ChessBoard_FindNextObstruction(board_data, &pdata->location,
          &mv[1], mvdir))
      continue;
    new_hit_piece = GET_BOARD_AT_IDX(board_data, mv[1]);
    if (!(new_hit_piece&ally_flag))
      continue;
    query.location = mv[1];
    new_hit = Graph_Get_Vertex(pgraph, &query);
    assert(NULL!=new_hit);
    assert(NULL!=new_hit->data);
    assert(Graph_Add_Edge(pgraph, curnode->idx, new_hit->idx, 0));
  }
  {
    GraphEdge_LL_t *mll = &moving_vert->adj_list;
    LL_CLOSE(GraphEdge, mll);
    assert(0UL==mll->nmemb);
    assert(NULL==mll->head);
    assert(NULL==mll->tail);
  }
  {
    ChessBoard_Idx_t origin = ((ChessPiece_Data_t*)moving_vert->data)->location;
    Mvmt_Dir_e mvmts[8] = {
      INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, 
      INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT, 
      INVALID_MVMT_FLAGBIT, INVALID_MVMT_FLAGBIT
    };
    int ct = 0;
    switch (mpiece&PIECE_IDX_MASK) {
    case PAWN_IDX:
      if (ally_flag&WHITE_FLAGBIT) {
        ChessBoard_Idx_t tmp[2] = {origin, origin};
        assert(ROW_8<move[1].coord.y);
        if (FILE_A < move[1].coord.x) {
          --tmp[0].coord.x;
          --tmp[0].coord.y;
        }
        if (FILE_H > move[1].coord.x) {
          ++tmp[1].coord.x;
          --tmp[1].coord.y;
        }
        for (int i = 0; i < 2; ++i) {
          if (tmp[i].raw == move[1].raw)
            continue;
          new_hit_piece = GET_BOARD_AT_IDX(board_data, tmp[i]);
          if (!(new_hit_piece&opp_flag))
            continue;
          query.location = tmp[i];
          new_hit = Graph_Get_Vertex(pgraph, &query);
          assert(NULL!=new_hit);
          assert(NULL!=new_hit->data);
          Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0);
        }
      }
      ct = 0;
      break;
    case BISHOP_IDX: 
      if (FILE_A < origin.coord.x) {
        if (ROW_8 < origin.coord.y) {
          mvmts[0] = UP_FLAGBIT|LEFT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
        }
        if (ROW_1 > origin.coord.y) {
          mvmts[1] = DOWN_FLAGBIT|LEFT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
        }
      }
      if (FILE_H > origin.coord.x) {
        if (ROW_8 < origin.coord.y) {
          mvmts[2] = UP_FLAGBIT|RIGHT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
        }
        if (ROW_1 > origin.coord.y) {
          mvmts[3] = DOWN_FLAGBIT|RIGHT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT;
        }
      }
      ct = 4;
      break;
    case ROOK_IDX:
      {
        if (FILE_A < origin.coord.x) {
          mvmts[0] = LEFT_FLAGBIT;
        }
        if (FILE_H > origin.coord.x) {
          mvmts[1] = RIGHT_FLAGBIT;
        }
        if (ROW_8 < origin.coord.y) {
          mvmts[2] = UP_FLAGBIT;
        }
        if (ROW_1 > origin.coord.y) {
          mvmts[3] = DOWN_FLAGBIT;
        }
      }
      ct = 4;
      break;
    case KNIGHT_IDX:
      {
        ChessBoard_Idx_t idxs[8];
        bool valid[8] = {0};
        if (FILE_A < origin.coord.x) {
          if (ROW_7 < origin.coord.y) {
            idxs[0].coord = (struct s_chess_coord){
              .x = origin.coord.x-1,
              .y = origin.coord.y-2
            };
            valid[0] = TRUE;
          }
          if (FILE_B < origin.coord.x) {
            if (ROW_8 < origin.coord.y) {
              idxs[1].coord = (struct s_chess_coord){
                .x=origin.coord.x-2,
                .y=origin.coord.y-1
              };
              valid[1] = TRUE;
            }
            if (ROW_1 > origin.coord.y) {
              idxs[2].coord = (struct s_chess_coord){
                .x = origin.coord.x-2,
                .y = origin.coord.y+1
              };
              valid[2] = TRUE;
            }
          }
          if (ROW_2 > origin.coord.y) {
            idxs[3].coord = (struct s_chess_coord){
              .x = origin.coord.x-1,
              .y = origin.coord.y+2
            };
            valid[3] = TRUE;
          }
        }
        if (FILE_H > origin.coord.x) {
          if (ROW_7 < origin.coord.y) {
            idxs[4].coord = (struct s_chess_coord){
              .x = origin.coord.x+1,
              .y = origin.coord.y-2
            };
            valid[4] = TRUE;
          }
          if (FILE_G > origin.coord.x) {
            if (ROW_8 < origin.coord.y) {
              idxs[5].coord = (struct s_chess_coord){
                .x = origin.coord.x+2,
                .y = origin.coord.y-1
              };
              valid[5] = TRUE;
            }
            if (ROW_1 > origin.coord.y) {
              idxs[6].coord = (struct s_chess_coord){
                .x = origin.coord.x+2,
                .y = origin.coord.y+1
              };
              valid[6] = TRUE;
            }
          }
          if (ROW_2 > origin.coord.y) {
            idxs[7].coord = (struct s_chess_coord){
              .x = origin.coord.x+1,
              .y = origin.coord.y+2
            };
            valid[7] = TRUE;
          }
        }
        for (int i = 0; i < 8; ++i) {
          if (!valid[i])
            continue;
          new_hit_piece = GET_BOARD_AT_IDX(board_data, idxs[i]);
          if (!(new_hit_piece&opp_flag))
            continue;
          query.location = idxs[i];
          new_hit = Graph_Get_Vertex(pgraph, &query);
          assert(NULL!=new_hit);
          assert(NULL!=new_hit->data);
          assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
        }
      }
      ct = 0;
      break;
    case QUEEN_IDX:
      if (FILE_A < origin.coord.x) {
        mvmts[0] = LEFT_FLAGBIT;
        mvmts[1] |= LEFT_FLAGBIT;
        mvmts[2] |= LEFT_FLAGBIT;
      }
      if (FILE_H > origin.coord.x) {
        mvmts[3] = RIGHT_FLAGBIT;
        mvmts[4] |= RIGHT_FLAGBIT;
        mvmts[5] |= RIGHT_FLAGBIT;
      }
      if (ROW_8 < origin.coord.y) {
        mvmts[6] = UP_FLAGBIT;
        mvmts[1] |= UP_FLAGBIT;
        mvmts[4] |= UP_FLAGBIT;
      }
      if (ROW_1 > origin.coord.y) {
        mvmts[7] = DOWN_FLAGBIT;
        mvmts[2] |= DOWN_FLAGBIT;
        mvmts[5] |= DOWN_FLAGBIT;
      }
      {
        Mvmt_Dir_e curmv;
        for (int i = 0, j; 6 > i; i+=3) {
          for (j = 1; 3 > j; ++j) {
            curmv = mvmts[i+j];
            if ((curmv&HOR_MASK) && (curmv&VER_MASK)) {
              mvmts[i+j]^=(DIAGONAL_MVMT_FLAGBIT|INVALID_MVMT_FLAGBIT);
            }
          }
        }
      }
      ct = 8;
      break;
    case KING_IDX:
      {
        ChessBoard_Idx_t idx = origin;
        int i, j, di;
        for (j = -1; 2 > j; ++j, idx.coord.y=origin.coord.y) {
          idx.coord.y += j;

          if (0UL!=(idx.coord.y&~7UL))
            continue;
          di = 0==j ? 2 : 1;
          for (i=-1; 2 > i; i+=di, idx.coord.x=origin.coord.x) {
            idx.coord.x+=i;
            if (0UL!=(idx.coord.x&~7UL))
              continue;
            new_hit_piece = GET_BOARD_AT_IDX(board_data, idx);
            if (!(new_hit_piece&opp_flag))
              continue;
            query.location = idx;
            new_hit = Graph_Get_Vertex(pgraph, &query);
            assert(NULL!=new_hit);
            assert(NULL!=new_hit->data);
            assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
          }
        }
      }
      ct = 0;
      break;
    default: assert(0);
    }
    if (0!=ct) {
      Mvmt_Dir_e curmv;
      for (int i = 0; i < 4; ++i) {
        curmv = mvmts[i];
        if (INVALID_MVMT_FLAGBIT==curmv)
          continue;
        if (!ChessBoard_FindNextObstruction(board_data, &origin, &query.location, curmv))
          continue;
        new_hit_piece = GET_BOARD_AT_IDX(board_data, query.location);
        if (!(new_hit_piece&opp_flag))
          continue;
        new_hit = Graph_Get_Vertex(pgraph, &query);
        assert(NULL!=new_hit);
        assert(NULL!=new_hit->data);
        assert(Graph_Add_Edge(pgraph, moving_vert->idx, new_hit->idx, 0));
      }
    }
  }
}
#endif

void ChessGame_UpdateBoardAndGraph(ChessGameCtx_t *ctx, Move_Validation_Flag_e move) {
  ChessPiece_Data_t query = {0};
  ChessPiece_Tracker_t *tracker = &ctx->tracker;
  Graph_t *pgraph = tracker->piece_graph;
  ChessBoard_Row_t *board = ctx->board_data;
  ChessBoard_Idx_t *mv = ctx->move_selections;
  GraphNode_t *cappiece, *moving_piece_vertex, *cur;
  GraphEdge_LL_t *ll;
  Obj_Attr_t *obj;
  ChessPiece_Data_t *moving, *capt;
  ChessPiece_e spiece;
  u32 whose_turn = ctx->whose_turn;

  spiece = board[BOARD_IDX(mv[0])];
  query.location = mv[0];
  moving_piece_vertex = 
    Graph_Get_Vertex(pgraph, &query);
  assert(NULL!=moving_piece_vertex);
  if (MOVE_CASTLE_MOVE_FLAGS_MASK&move) {
    ChessBoard_Idx_t rook_moves[2] = {mv[0], mv[1]};
    GraphNode_t *rook_vert;
    const ChessPiece_Data_t *RDATA;
    ChessPiece_Data_t *new_rdata;
    ChessPiece_e *rook_start_boardp, *rook_end_boardp;
    const ChessPiece_e ALLY_ROOK = (ROOK_IDX|whose_turn);
    switch (MOVE_CASTLE_MOVE_FLAGS_MASK&move) {
    case MOVE_CASTLE_KINGSIDE:
      rook_moves[0].coord.x = FILE_H;
      --rook_moves[1].coord.x;
      break;
    case MOVE_CASTLE_QUEENSIDE:
      rook_moves[0].coord.x = FILE_A;
      ++rook_moves[1].coord.x;
      break;
    default:
      assert(MOVE_CASTLE_MOVE_FLAGS_MASK!=(move&MOVE_CASTLE_MOVE_FLAGS_MASK));
      exit(1);
    }
    query.location = rook_moves[0];
    rook_vert = Graph_Get_Vertex(pgraph, &query);
    assert(NULL!=rook_vert);
    assert(NULL!=rook_vert->data);
    RDATA = (const ChessPiece_Data_t*)rook_vert->data;
    assert(RDATA->location.raw==query.location.raw);
    rook_start_boardp = &GET_BOARD_AT_IDX(board,query.location);
    rook_end_boardp = &GET_BOARD_AT_IDX(board, rook_moves[1]);
    assert(ALLY_ROOK==*rook_start_boardp
                  && EMPTY_IDX==*rook_end_boardp);
    *rook_end_boardp = ALLY_ROOK;
    *rook_start_boardp = EMPTY_IDX;
    query = *RDATA;
    query.location = rook_moves[1];
    assert(Graph_Update_Vertex_Data(pgraph, rook_vert->idx, &query));
    new_rdata = rook_vert->data;
    assert(RDATA!=new_rdata);
    assert(NULL!=new_rdata);
    RDATA = NULL;
    obj = new_rdata->obj_ctl;
    UPDATE_PIECE_SPRITE_LOCATION(obj, rook_moves[1]);
    OAM_Copy(&OAM_ATTR[new_rdata->roster_id], obj, 1);
  }else if (MOVE_CAPTURE&move) {
    if (move&MOVE_EN_PASSENT) {
      ChessPiece_e epiece = board[BOARD_IDX(mv[1])];
      assert(EMPTY_IDX==(epiece&PIECE_IDX_MASK));
      query.location.coord = (struct s_chess_coord){
        .x = ctx->move_selections[1].coord.x,
        .y = ctx->move_selections[0].coord.y,
      };
      board[BOARD_IDX(query.location)] = EMPTY_IDX;
    } else {
      ChessPiece_e epiece = board[BOARD_IDX(mv[1])];
      assert((whose_turn^PIECE_TEAM_MASK)&epiece);
      assert(EMPTY_IDX!=(epiece&PIECE_IDX_MASK));
      query.location = mv[1];
    }
    cappiece = Graph_Get_Vertex(pgraph, &query);
    assert(NULL!=cappiece);
    capt = (ChessPiece_Data_t*)cappiece->data;
    tracker->roster.all ^= 1<<capt->roster_id;
    if (whose_turn&WHITE_FLAGBIT) {
      tracker->pieces_captured[0][tracker->capcount[0]++] = capt->obj_ctl;
      qsort(tracker->pieces_captured[0], tracker->capcount[0], sizeof(void*), ObjAttrCmp);
    } else {
      tracker->pieces_captured[1][tracker->capcount[1]++] = capt->obj_ctl;
      qsort(tracker->pieces_captured[1], tracker->capcount[1], sizeof(void*), ObjAttrCmp);
    }
    Graph_LazyDelete_Vertex(pgraph, cappiece->idx);
    ChessBoard_DrawCapturedTeam(&ctx->obj_data.pieces[0][0], tracker, whose_turn^PIECE_TEAM_MASK);
  }
  board[BOARD_IDX(mv[0])] = EMPTY_IDX;
  board[BOARD_IDX(mv[1])] = spiece;
  moving = (ChessPiece_Data_t*)moving_piece_vertex->data;
  obj = moving->obj_ctl;
  if ((spiece&PIECE_IDX_MASK)!=BOARD_BACK_ROWS_INIT[moving->roster_id]) {
    obj = moving->obj_ctl;
    obj->attr2.sprite_idx = (spiece&PIECE_IDX_MASK)*TILES_PER_CSPR*2;
    if (spiece&BLACK_FLAGBIT)
      obj->attr2.sprite_idx += Chess_sprites_Glyph_Count*TILES_PER_CSPR*2;
  }
  UPDATE_PIECE_SPRITE_LOCATION(obj, mv[1]);
  OAM_Copy(&OAM_ATTR[moving->roster_id], obj, 1);
  query = *moving;
  query.location = mv[1];

  Graph_Update_Vertex_Data(pgraph, moving_piece_vertex->idx, &query);
#ifndef UPDATE_ALL
  ChessGame_UpdateMovedPieceEdges(ctx->board_data, pgraph, ctx->move_selections, moving_piece_vertex);
  for (int i = 0, cur_opp_team_ofs; i < CHESS_TEAM_PIECE_COUNT; ++i) {
    if (i&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT) {
      cur_opp_team_ofs = BLACK_FLAGBIT;
    } else {
      cur_opp_team_ofs = WHITE_FLAGBIT;
    }
    cur = &pgraph->vertices[i];
  }
#else
  for (int i = 0; i < CHESS_TEAM_PIECE_COUNT * 2; ++i) {
    cur = &pgraph->vertices[i];
    if (NULL == cur->data)
      continue;
    ll = &cur->adj_list;
    LL_CLOSE(GraphEdge, ll);
    ChessGame_UpdateVertexEdges(board, pgraph, cur);
  }
#endif
}



u32 ChessGameLoop(ChessGameCtx_t *ctx) {
  u32 ret = 0;
  Move_Validation_Flag_e move;
  while (Vsync(), !ret) {
    do move = ChessGame_GetMove(ctx); while (MOVE_UNSUCCESSFUL==move);
    ChessGame_UpdateBoardAndGraph(ctx, move);
    ChessGame_UpdateMoveHistory(ctx, move);
    ctx->whose_turn ^= PIECE_TEAM_MASK;
  }
  return ret;
}

void UPDATE_PIECE_SPRITE_LOCATION(Obj_Attr_t *spr_obj, ChessBoard_Idx_t move) {
  spr_obj->attr0.regular.y 
    = CHESS_BOARD_Y_OFS + move.coord.y*Chess_sprites_Glyph_Height;
  spr_obj->attr1.regular.x
    = CHESS_BOARD_X_OFS + move.coord.x*Chess_sprites_Glyph_Width;
}
