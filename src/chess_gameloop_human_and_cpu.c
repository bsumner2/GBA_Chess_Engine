#include <GBAdev_types.h>
#include <GBAdev_functions.h>
#include <GBAdev_memdef.h>
#include <GBAdev_memmap.h>
#include <GBAdev_util_macros.h>
#include <stdlib.h>
#include "chess_ai.h"
#include "chess_board_state.h"
#include "debug_io.h"
#include "graph.h"
#include "key_status.h"
#include "chess_board.h"
#include "chess_gameloop.h"
#include "chess_obj_sprites_data.h"
#include "chess_sprites.h"
#include "linked_list.h"


#define UPDATE_ALL


static void ChessGame_AIXHuman_PromotionPrompt(ChessGameCtx_t *ctx,
                                          ChessPiece_Data_t *pawn,
                                          const ChessMoveIteration_t *ai_move);
static ChessMoveIteration_t ChessGame_AIXHuman_GetMove(ChessGameCtx_t *ctx,
                                                       ChessAI_Params_t *ai,
                                                       BOOL retry);

static void ChessGame_AIXHuman_UpdateBoardAndGraph(ChessGameCtx_t *ctx, 
                                         BOOL *promotion_occurred,
                                         const ChessMoveIteration_t *move_data,
                                         BOOL ais_turn);

static void ChessGame_AIXHuman_UpdateVertexEdges(const ChessBoard_t board_data, 
                                        Graph_t *pgraph,
                                        GraphNode_t *moving_vert);
static void ChessGame_RestoreSpritesToCtxLayout(const ChessGameCtx_t *ctx);

#ifdef _DEBUG_BUILD_
#define _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_
#endif

#ifdef _AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_
extern IWRAM_CODE void ChessAI_SpriteDataFromCtx(const ChessGameCtx_t *ctx);
#else
#define ChessAI_SpriteDataFromCtx(dummy_field)
#endif  /* defined(_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_) */

#define PROMOTION_SEL_MASK 3
extern const ChessPiece_e PROMOTION_SEL[4];

void ChessGame_RestoreSpritesToCtxLayout(const ChessGameCtx_t *ctx) {
  OAM_Copy(OAM_ATTR, ctx->obj_data.pieces[0], CHESS_TOTAL_PIECE_COUNT);
  // Have to do two separate calls, since they're aligned weirdly to keep
  // OAM_Init functional, which relies on 4 OAM alignment to simultaneously
  // initialize both OAM attrs and the affine transform data entries 
  // interleaved between them
  OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS], ctx->obj_data.sels, 2);
}

void ChessGame_AIXHuman_PromotionPrompt(ChessGameCtx_t *ctx,
                                        ChessPiece_Data_t *pawn,
                                        const ChessMoveIteration_t *ai_move) {
  const u32 spr_ofs = ctx->whose_turn&WHITE_FLAGBIT
                          ?0:2*TILES_PER_CSPR*Chess_sprites_Glyph_Count;
  Obj_Attr_t *obj = pawn->obj_ctl;
  ChessBoard_Idx_t *move = ctx->move_selections;
  int promotion_id=0, pawn_id = pawn->roster_id;
  obj->attr2.sprite_idx = PROMOTION_SEL[promotion_id]*2*CSPR_TILES_PER_DIM
                                        + spr_ofs;
  UPDATE_PIECE_SPRITE_LOCATION(obj, move[1]);
  OAM_Copy(&OAM_ATTR[pawn_id], obj, 1);
  if (NULL!=ai_move) {
    BOOL valid_ai_promo_flag;
    promotion_id = ai_move->promotion_flag;
    switch ((ChessPiece_e)promotion_id) {
    case BISHOP_IDX:
    case ROOK_IDX:
    case KNIGHT_IDX:
    case QUEEN_IDX:
      valid_ai_promo_flag = TRUE;
      break;
    default:
      valid_ai_promo_flag = FALSE;
      break;
    }
    assert(valid_ai_promo_flag);

    obj->attr2.sprite_idx = spr_ofs
                            + promotion_id*2*TILES_PER_CSPR;
    OAM_Copy(&OAM_ATTR[pawn_id], obj, 1);
    ctx->board_data[BOARD_IDX(move[0])]
                      = (ctx->whose_turn|promotion_id);
    return;
  }
  for (BOOL sel = FALSE, sprchange=TRUE; !sel; 
       sel=(0!=KEY_STROKE(A)), sprchange=TRUE) {
    IRQ_Sync(IRQ_FLAG(KEYPAD));
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
    obj->attr2.sprite_idx = spr_ofs
                            + PROMOTION_SEL[promotion_id]*2*TILES_PER_CSPR;
    OAM_Copy(&OAM_ATTR[pawn_id], obj, 1);
  }

  ctx->board_data[BOARD_IDX(move[0])]
                    = (ctx->whose_turn|PROMOTION_SEL[promotion_id]);
}

ChessMoveIteration_t ChessGame_AIXHuman_GetMove(ChessGameCtx_t *ctx,
                                                ChessAI_Params_t *ai,
                                                BOOL retry) {
  Obj_Attr_t *sels = ctx->obj_data.sels;
  ChessBoard_Idx_t curmove={0}, *move = ctx->move_selections;
  ChessMoveIteration_t ret;
  Move_Validation_Flag_e move_result;
  if (NULL!=ai) {
    ChessAI_MoveSearch_Result_t result;
    ChessAI_SpriteDataFromCtx(ctx);
    ChessAI_Move(ai, &result);
    move[0] = BOARD_IDX_CONVERT(result.start, NORMAL_IDX_TYPE);
    move[1] = BOARD_IDX_CONVERT(result.dst, NORMAL_IDX_TYPE);
    ChessGame_RestoreSpritesToCtxLayout(ctx);
    move_result = ChessBoard_ValidateMove(ctx);
    ensure(move_result&MOVE_SUCCESSFUL,
        "(move_result:=ChessBoard_ValidateMove(ctx) did not yield successful "
        "move. Move attempted:\n"
        "result::ChessAI_MoveSearch_Result_t = {\n\t"
        ".start.raw = ( \x1b[0x44E4]%01hhu\x1b[0x1484], \x1b[0x44E4]%hhu"
        "\x1b[0x1484] )\n\t.dst.raw = ( \x1b[0x44E4]%01hhu\x1b[0x1484], "
        "\x1b[0x44E4]%hhu\x1b[0x1484] )\n\t.mv_flags = \x1b[0x44E4]0x%04hX"
        "\x1b[0x1484]\n\t.promo = \x1b[0x44E4]%u\x1b[0x1484]\n\t"
        ".score = \x1b[0x44E4]%d\x1b[0x1484]\n}\n"
        "Piece tried to move according to ctx: \x1b[0x44E4]%s\x1b[0x1484]\n"
        "Piece tried to move according to ai: \x1b[0x44E4]%s\n"
        "\x1b[0x0000](For debugging):\x1b[0x1484]\n\tai_params->root_state = "
        "\x1b[0x44E4]%p\x1b[0x1484]",
        result.start.coord.x, result.start.coord.y, result.dst.coord.x,
        result.dst.coord.y, result.mv_flags, result.promo, result.score,
        DebugIO_ChessPiece_ToString(ctx->board_data[BOARD_IDX(result.start)]),
        DebugIO_ChessPiece_ToString(BoardState_GetPiece_CompactIdx(
                                                                ai->root_state,
                                                                result.start)),
        (void*)(ai->root_state));
    result.mv_flags|=MOVE_SUCCESSFUL;
    ensure(move_result==result.mv_flags,
        "move_result, set to value returned by ChessBoard_ValidateMove(ctx),\n"
        "Did not match value within AI's returned "
        "ChessAI_MoveSearch_Result_t::mv_flags\n\tMove Result = \x1b[0x44E4]"
        "0x%04hX\x1b[0x1484]\n\tAI's mv flags: \x1b[0x44E4]0x%04hX\x1b[0x1484]",
        move_result, result.mv_flags);

    ensure(result.mv_flags&MOVE_SUCCESSFUL,
        "result = {\n\t"
        ".start.raw = 0x\x1b[0x44E4]%01hhX\x1b[0x44E4]%01hhX\x1b[0x1484]\n\t"
        ".dst.raw = \x1b[0x6756]0x\x1b[0x44E4]%01hhX\x1b[0x44E4]%01hhX"
        "\x1b[0x1484]\n\t.mv_flags = \x1b[0x44E4]0x%04hX\x1b[0x1484]\n\t"
        ".promo = \x1b[0x44E4]%u\x1b[0x1484]\n\t.score = \x1b[0x44E4]%d"
        "\x1b[0x1484]\n}\n",
        result.start.coord.x, result.start.coord.y, result.dst.coord.x,
        result.dst.coord.y, result.mv_flags, result.promo, result.score);
    
    ret.special_flags = result.mv_flags;
    ret.promotion_flag = result.promo;
    ret.dst = move[1];
    OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS], sels, 2);
    return ret;
  }
  if (!retry) {
    sels[0].attr0 = (Obj_Attr_Fields0_t) {
      .regular = {
        .y = CHESS_BOARD_Y_OFS,
        .mode = OBJ_GFX_MODE_BLEND,
        .pal_8bpp = TRUE,
      }
    };
 
    if (ctx->whose_turn&WHITE_FLAGBIT) {
      sels[0].attr0.regular.y 
                    += Chess_sprites_Glyph_Height*(CHESS_BOARD_ROW_COUNT-1);
      curmove.coord.y = ROW_1;
    }
    sels[0].attr1.regular.x = CHESS_BOARD_X_OFS;
    sels[0].attr2.sprite_idx = EMPTY_IDX*TILES_PER_CSPR*2;
    sels[1] = sels[0];
  } else {
    curmove = move[1];
  }
  for (int i=retry?1:0; 2>i; ++i, Vsync()) {
    OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS+i], &sels[i], 1);
    for (BOOL sel=FALSE; 
         !sel; 
         OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS + i], &sels[i], 1),
         Vsync()) {
      IRQ_Sync(IRQ_FLAG(KEYPAD));
      if ((sel=KEY_STROKE(A))) {
        continue;
      } else if (KEY_STROKE(B)) {
        if (1==i) {
          sels[1].attr0.regular.disable = TRUE;
          OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS+1], &sels[1], 1);
          sels[1] = sels[0];
          curmove = move[0];
          --i;
          continue;
        }
      }
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
    if (0==i) {
      if (0==(ctx->whose_turn&GET_BOARD_AT_IDX(ctx->board_data, curmove))) {
        --i;
        ChessGame_NotifyInvalidStart(sels);
        curmove.raw=0ULL;
        if (ctx->whose_turn&WHITE_FLAGBIT) {
          curmove.coord.y = ROW_1;
        }
        UPDATE_PIECE_SPRITE_LOCATION(&sels[0], curmove);
        continue;
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

  ret.special_flags = ChessBoard_ValidateMove(ctx);
  ret.dst = move[1];
  return ret;
}

void ChessGame_AIXHuman_UpdateVertexEdges(const ChessBoard_t board_data,
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

void ChessGame_AIXHuman_UpdateBoardAndGraph(ChessGameCtx_t *ctx, 
                                         BOOL *promotion_occurred,
                                         const ChessMoveIteration_t *move_data,
                                         BOOL ais_turn) {
  ChessPiece_Data_t query = {0}, 
                    capt_static;  /* Gonna need a static copy for after 
                                     captured piece's vertex is deleted */
  ChessPiece_Tracker_t *tracker = &ctx->tracker;
  Graph_t *pgraph = tracker->piece_graph;
  ChessBoard_Row_t *board = ctx->board_data;
  ChessBoard_Idx_t *mv = ctx->move_selections;
  GraphNode_t *cappiece, *moving_piece_vertex, *cur;
  GraphEdge_LL_t *ll;
  ChessPiece_Data_t *moving, *capt = NULL;
  u32 whose_turn = ctx->whose_turn;
  Move_Validation_Flag_e move = move_data->special_flags;
  ChessPiece_e spiece;

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
    assert(NULL!=new_rdata);
    assert(rook_moves[1].raw==new_rdata->location.raw);
    RDATA = NULL;
    capt = new_rdata;
  } else if (MOVE_CAPTURE&move) {
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
    capt_static = *capt;
    Graph_LazyDelete_Vertex(pgraph, cappiece->idx);
    capt = &capt_static;
  }

  if (PAWN_IDX==(spiece&PIECE_IDX_MASK)) {
    BOOL promo_occurred = FALSE;
    if (ctx->whose_turn&WHITE_FLAGBIT) {
      if (ctx->move_selections[1].coord.y==ROW_8) {
        promo_occurred = TRUE;
      }
    } else if (ctx->move_selections[1].coord.y==ROW_1) {
      promo_occurred = TRUE;
    }

    *promotion_occurred = promo_occurred;
    if (promo_occurred) {
      moving = (ChessPiece_Data_t*)moving_piece_vertex->data;
      ChessGame_AnimateMove(ctx, moving, capt, move);
      ChessGame_AIXHuman_PromotionPrompt(ctx,
                                         moving,
                                         ais_turn?move_data:NULL);
      spiece = board[BOARD_IDX(mv[0])];
      board[BOARD_IDX(mv[0])] = EMPTY_IDX;
      board[BOARD_IDX(mv[1])] = spiece;
      query = *moving;
      query.location = mv[1];
      assert(Graph_Update_Vertex_Data(pgraph, moving_piece_vertex->idx, &query));
      /* Update moving to point to vertex's new data. */
      moving = moving_piece_vertex->data;
      if (NULL!=capt) {
        capt_static.obj_ctl->attr0.regular.disable = FALSE;
        ChessGame_DrawCapturedTeam(&ctx->obj_data.pieces[0][0],
                                   tracker,
                                   whose_turn^PIECE_TEAM_MASK);
      }
    }
  } else {
    *promotion_occurred = FALSE;
  }

  if (!*promotion_occurred) {
    board[BOARD_IDX(mv[0])] = EMPTY_IDX;
    board[BOARD_IDX(mv[1])] = spiece;
    moving = (ChessPiece_Data_t*)moving_piece_vertex->data;
    query = *moving;
    query.location = mv[1];
    assert(Graph_Update_Vertex_Data(pgraph, moving_piece_vertex->idx, &query));
    /* Update moving to point to vertex's new data. */
    moving = moving_piece_vertex->data;
  
    ChessGame_AnimateMove(ctx, moving, capt, move);
    if (NULL!=capt && 0==(move&MOVE_CASTLE_MOVE_FLAGS_MASK)) {
      capt_static.obj_ctl->attr0.regular.disable = FALSE;
      ChessGame_DrawCapturedTeam(&ctx->obj_data.pieces[0][0], tracker, whose_turn^PIECE_TEAM_MASK);
    }
  }
  ctx->obj_data.sels[0].attr0.regular.disable = TRUE;
  ctx->obj_data.sels[1].attr0.regular.disable = TRUE;
  OAM_Copy(&OAM_ATTR[SEL_OAM_IDX_OFS], ctx->obj_data.sels, 2);
  for (int i = 0; i < CHESS_TEAM_PIECE_COUNT * 2; ++i) {
    cur = &pgraph->vertices[i];
    if (NULL == cur->data)
      continue;
    ll = &cur->adj_list;
    LL_CLOSE(GraphEdge, ll);
    ChessGame_AIXHuman_UpdateVertexEdges(board, pgraph, cur);
  }
}

u32 ChessGame_AIXHuman_Loop(ChessGameCtx_t *ctx,
                           ChessAI_Params_t *ai) {
  u32 ret = 0;
  const u32 AI_TEAM_FLAG = CONVERT_CHESS_AI_TEAM_FLAG(ai->team);
  if (!(AI_TEAM_FLAG&PIECE_TEAM_MASK)) {
    DebugMsgF(DEBUG_MSG_EXITS,
              "AI_TEAM_FLAG = \x1b[0x44E4]0x%04lX\x1b[0x1484]\n"
              "ai->team = \x1b[0x44E4]0x%02hhX\x1b[0x1484]\n",
              AI_TEAM_FLAG, ai->team);
  }
  ChessPiece_Data_t *checking_pcs = NULL;
  int checking_pcs_ct;
  ChessMoveIteration_t move;
  BOOL promotion_occurred, ais_turn;
  while (Vsync(), !ret) {
    ais_turn = AI_TEAM_FLAG==ctx->whose_turn;
    if (ais_turn) {
      move = ChessGame_AIXHuman_GetMove(ctx, ai, FALSE);
      assert(MOVE_SUCCESSFUL&move.special_flags);
    } else {
      for (move = ChessGame_AIXHuman_GetMove(ctx, NULL, FALSE)
            ; ;
           move = ChessGame_AIXHuman_GetMove(ctx, NULL, TRUE)) {
        if (MOVE_SUCCESSFUL&move.special_flags)
          break;
        ChessGame_NotifyInvalidDest(ctx->obj_data.sels);
      }
    }
    ChessGame_AIXHuman_UpdateBoardAndGraph(ctx,
                                           &promotion_occurred,
                                           &move,
                                           ais_turn);
    ChessGame_UpdateMoveHistory(ctx,
                                move.special_flags,
                                promotion_occurred);
    checking_pcs_ct = ChessBoard_KingInCheck(&checking_pcs,
        &ctx->tracker,
        ctx->whose_turn^PIECE_TEAM_MASK);
    if (0!=checking_pcs_ct) {
      assert(0 < checking_pcs_ct);
      if (ChessGame_CalculateIfCheckmate(ctx, checking_pcs, checking_pcs_ct)) {
        ret = ctx->whose_turn;
        free(checking_pcs);
        continue;
      }
      free(checking_pcs);
      checking_pcs = NULL;
      checking_pcs_ct = 0;
    } else {
      if (ChessGame_CalculateIfStalemate(ctx)) {
        assert(0==checking_pcs_ct && NULL==checking_pcs);
        return 0;
      }
    }

    assert(0==checking_pcs_ct && NULL==checking_pcs);
    ctx->whose_turn ^= PIECE_TEAM_MASK;
    ai->root_state =BoardState_FromCtx(ai->root_state, ctx);
  }
  return ret;
}
