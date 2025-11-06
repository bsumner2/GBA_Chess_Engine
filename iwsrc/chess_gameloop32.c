/* Copyright (C) 2025 Burt O Sumner Claim of Authorship Rights Reserved
 * Allowed to edit, copy, redistribute, or otherwise use at your discretion,
 * with the only caveat being that this comment remains unmoved, 
 * untouched/unedited, and visible in any redists.
 **/

/* @brief The non-linear movement behaviour of the knight is so different from
 * any of the other pieces, which all, otherwise have linear movement, that
 * knight should have its own special case mvmt function, just to keep the
 * code cleaner. Knight animations are going to use a parabolic movement path.
 * Given the maths-heavy trajectory mapping, this function will have to be in
 * working ram.
 * */

#define __TRANSPARENT_BOARD_STATE__
#include <GBAdev_functions.h>
#include <GBAdev_memmap.h>
#include <GBAdev_util_macros.h>
#include "chess_board.h"
IWRAM_CODE void ChessGame_AnimateKnightMove(
                                      __INTENT__(UNUSED) ChessGameCtx_t *ctx,
                                      ChessPiece_Data_t *moving,
                                      __INTENT__(UNUSED) ChessBoard_Idx_t *mv,
                                      Knight_Mvmt_Dir_e kdir) {
  static const float TALL_YPRIME_KOEFF=(32.F/200.F),
                     TALL_YPRIME_KONST=(3.2F*256.F);
  static const float WIDE_XPRIME_KOEFF=(5/16.F);
  static const i32   WIDE_XPRIME_KONST=(5<<8);
  Obj_Attr_t *const obj = moving->obj_ctl;
  const i32 XORIGIN=obj->attr1.regular.x, YORIGIN=obj->attr0.regular.y;
  i32 xoff;
  i32 yoff;
  Knight_Mvmt_Dir_e hor_dir = kdir&HOR_MASK, ver_dir = kdir&VER_MASK;
  if (KNIGHT_MVMT_TALL_FLAGBIT==(KNIGHT_MVMT_DIM_MASK&kdir)) {

    for (xoff=yoff=0; xoff < 5120; xoff+=256) {
      yoff += ((i32)(TALL_YPRIME_KONST-xoff*TALL_YPRIME_KOEFF))>>8;
      switch (ver_dir) {
      case KNIGHT_MVMT_DOWN_FLAGBIT:
        obj->attr0.regular.y = YORIGIN+yoff;
        break;
      case KNIGHT_MVMT_UP_FLAGBIT:
        obj->attr0.regular.y = YORIGIN-yoff;
        break;
      default:
        assert(kdir&VER_MASK);
        assert(0);  // default case should ideally never happen, so in order to 
                    // debug, force assertion failure with assert(*FALSE*);
      }
      switch (hor_dir) {
        case KNIGHT_MVMT_RIGHT_FLAGBIT:
        obj->attr1.regular.x = XORIGIN+(xoff>>8);
        break;
      case KNIGHT_MVMT_LEFT_FLAGBIT:
        obj->attr1.regular.x = XORIGIN-(xoff>>8);
        break;
      default:  // Same deal here
        assert(kdir&HOR_MASK);
        assert(0);
      }
/*      obj->attr0.regular.y = YORIGIN+yoff;
      obj->attr1.regular.x = XORIGIN+(xoff>>8);*/
      OAM_Copy(OAM_ATTR+moving->roster_id, obj, 1);
      SUPERVISOR_CALL(0x05);
    }
    return;
  }
  // **else**
  for (xoff=yoff=0; yoff < 5120; yoff+=256) {
    xoff+= ((i32)(WIDE_XPRIME_KONST-WIDE_XPRIME_KOEFF*yoff))>>8;
    switch (ver_dir) {
    case KNIGHT_MVMT_DOWN_FLAGBIT:
      obj->attr0.regular.y = YORIGIN+(yoff>>8);
      break;
    case KNIGHT_MVMT_UP_FLAGBIT:
      obj->attr0.regular.y = YORIGIN-(yoff>>8); 
      break;
    default:
      assert(kdir&VER_MASK);
      assert(0);
    }
    switch (hor_dir) {
    case KNIGHT_MVMT_RIGHT_FLAGBIT:
      obj->attr1.regular.x = XORIGIN+xoff;
      break;
    case KNIGHT_MVMT_LEFT_FLAGBIT:
      obj->attr1.regular.x = XORIGIN-xoff;
      break;
    default:
      assert(kdir&HOR_MASK);
      assert(0);
    }
    OAM_Copy(OAM_ATTR+moving->roster_id, obj, 1);
    SUPERVISOR_CALL(0x05);
  }
}
