
#include "chess_board.h"

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


