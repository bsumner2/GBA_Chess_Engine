/** (C) 19 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_GAMELOOP_
#define _CHESS_GAMELOOP_

#include <GBAdev_functions.h>
#include "chess_ai.h"
#include "chess_board.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#define ChessGame_NotifyInvalidDest(mvmt) ChessGame_NotifyInvalidMove(mvmt, 1)
#define ChessGame_NotifyInvalidStart(mvmt) ChessGame_NotifyInvalidMove(mvmt, 0)
#define SEL_OAM_IDX_OFS (sizeof(ChessObj_Pieces_t)/sizeof(Obj_Attr_t))
INLN void UPDATE_PIECE_SPRITE_LOCATION(Obj_Attr_t *spr_obj, 
                                       ChessBoard_Idx_t move);
INLN void Vsync(void);


void ChessBG_Init(void);

IWRAM_CODE void ChessGameloop_ISR_Handler(void);

void ChessGameCtx_Init(ChessGameCtx_t *ctx);
void ChessGameCtx_Close(ChessGameCtx_t *ctx);

u32 ChessGame_HumanXHuman_Loop(ChessGameCtx_t *ctx);
u32 ChessGame_AIXHuman_Loop(ChessGameCtx_t *ctx, ChessAI_Params_t *ai);

void ChessGame_AnimateMove(ChessGameCtx_t *ctx,
                           ChessPiece_Data_t *moving,
                           ChessPiece_Data_t *captured,
                           Move_Validation_Flag_e special_flags);

BOOL ChessGame_CalculateIfCheckmate(
                                 const ChessGameCtx_t *ctx,
                                 const ChessPiece_Data_t *checking_pcs,
                                 int checking_pcs_ct);

BOOL ChessGame_CalculateIfStalemate(const ChessGameCtx_t *ctx);

void ChessGame_DrawCapturedTeam(const Obj_Attr_t *obj_origin,
                                ChessPiece_Tracker_t *tracker, 
                                u32 captured_team);

void IRQ_Sync(u32 flags);

void ChessGame_NotifyInvalidMove(Obj_Attr_t *mvmt, int idx);

void ChessGame_UpdateMoveHistory(ChessGameCtx_t *ctx,
                                 Move_Validation_Flag_e move_outcome,
                                 BOOL promotion_occurred);

int ObjAttrCmp(const void *a, const void *b);


void UPDATE_PIECE_SPRITE_LOCATION(Obj_Attr_t *spr_obj, ChessBoard_Idx_t move) {
  spr_obj->attr0.regular.y 
    = CHESS_BOARD_Y_OFS + move.coord.y*Chess_sprites_Glyph_Height;
  spr_obj->attr1.regular.x
    = CHESS_BOARD_X_OFS + move.coord.x*Chess_sprites_Glyph_Width;
}

void Vsync(void) {
  SUPERVISOR_CALL(0x05);
}





#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_GAMELOOP_ */
