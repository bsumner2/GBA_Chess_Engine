/** (C) Burt Sumner 2025 */
#ifndef _CHESS_BOARD_H_
#define _CHESS_BOARD_H_

#include "chess_sprites.h"
#include "graph.h"
#include <GBAdev_util_macros.h>
#include <GBAdev_types.h>
#include <GBAdev_memdef.h>
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

typedef enum e_move_validation_flags {
  MOVE_EN_PASSENT=0x8000,
  MOVE_CASTLE_QUEENSIDE=0x4000,
  MOVE_CASTLE_KINGSIDE=0x2000,
  MOVE_PAWN_TWO_SQUARE=0x1000,
  MOVE_CASTLE_MOVE_FLAGS_MASK=0x6000,
  MOVE_SPECIAL_MOVE_FLAGS_MASK=0xF000,
  MOVE_CAPTURE=0x0800,

  MOVE_SUCCESSFUL=TRUE,
  MOVE_UNSUCCESSFUL=FALSE,
} Move_Validation_Flag_e;

typedef enum e_piece {
  WHITE_FLAGBIT = 0x1000,
  BLACK_FLAGBIT=0x2000,
  PIECE_TEAM_MASK=0x3000,
  PIECE_IDX_MASK=0x0007,
  PAWN_IDX=0x0000,
  BISHOP_IDX=0x0001,
  ROOK_IDX=0x0002,
  KNIGHT_IDX=0x0003,
  QUEEN_IDX=0x0004,
  KING_IDX=0x0005,
  EMPTY_IDX=0x0006,
  BLACK_PAWN=0x2000,
  BLACK_BISHOP=0x2001,
  BLACK_ROOK=0x2002,
  BLACK_KNIGHT=0x2003,
  BLACK_QUEEN=0x2004,
  BLACK_KING=0x2005,
  WHITE_PAWN=0x1000,
  WHITE_BISHOP=0x1001,
  WHITE_ROOK=0x1002,
  WHITE_KNIGHT=0x1003,
  WHITE_QUEEN=0x1004,
  WHITE_KING=0x1005,
} ALIGN(2) ChessPiece_e;


extern const ChessPiece_e BOARD_BACK_ROWS_INIT[16];

typedef enum ALIGN(4) e_row {
  ROW_8 = 0,
  ROW_7,
  ROW_6,
  ROW_5,
  ROW_4,
  ROW_3,
  ROW_2,
  ROW_1,
  IGNORE_ROW_ENUM_ALIGNMENT_FORCER=0xFFFFFFFFUL
} ChessBoard_Row_e;

#define CHESSBOARD_ROW_ENUM_VALIDITY_MASK   7
#define CHESSBOARD_FILE_ENUM_VALIDITY_MASK  7

typedef enum ALIGN(4) e_file {
  FILE_A=0,
  FILE_B,
  FILE_C,
  FILE_D,
  FILE_E,
  FILE_F,
  FILE_G,
  FILE_H,
  IGNORE_FILE_ENUM_ALIGNMENT_FORCER=0xFFFFFFFFUL
} ChessBoard_File_e;

static_assert(sizeof(enum e_file)==4);
static_assert(sizeof(enum e_row)==4);

typedef enum e_chess_piece_roster_id {
  PIECE_ROSTER_ABS_ID_MASK=31, PIECE_ROSTER_ID_MASK=15,  PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT=16,
  ROOK0=0, KNIGHT0, BISHOP0, QUEEN, KING, BISHOP1, KNIGHT1, ROOK1,
  PAWN0, PAWN1, PAWN2, PAWN3, PAWN4, PAWN5, PAWN6, PAWN7
} ChessPiece_Roster_Id_e;

#define FILE(file) FILE_##file
#define ROW(row) ROW##row

typedef enum e_mvmt_dir {
  UP_FLAGBIT=0x0002, DOWN_FLAGBIT=0x0001, LEFT_FLAGBIT=0x0008, RIGHT_FLAGBIT=0x0004,
  HOR_MASK=0x000C, VER_MASK=0x0003, DIAGONAL_MVMT_FLAGBIT=0x0010, 
  KNIGHT_MVMT_FLAGBIT=0x0020, INVALID_MVMT_FLAGBIT=0x0080
} Mvmt_Dir_e;

typedef union u_board_idx {
  struct s_chess_coord {
    ChessBoard_File_e x;
    ChessBoard_Row_e y;
  } coord;
  u64 raw;
} ChessBoard_Idx_t;

#define CHESS_BOARD_FILE_COUNT 8
#define CHESS_BOARD_ROW_COUNT  8
#define CHESS_BOARD_SQUARE_COUNT (CHESS_BOARD_FILE_COUNT*CHESS_BOARD_ROW_COUNT)
#define CHESS_TEAM_PIECE_COUNT (CHESS_BOARD_FILE_COUNT*2)

#define BLACK_TURN_FLAGBIT BLACK_FLAGBIT
#define WHITE_TURN_FLAGBIT WHITE_FLAGBIT

typedef enum e_piece ChessBoard_Row_t[CHESS_BOARD_FILE_COUNT];
typedef Obj_Attr_t ChessObj_Team_t[CHESS_TEAM_PIECE_COUNT];

typedef ChessBoard_Row_t ChessBoard_t[CHESS_BOARD_ROW_COUNT];
typedef ChessObj_Team_t  ChessObj_Pieces_t[2];

typedef Obj_Attr_t ChessObj_Mvmt_Sel_t[2];

typedef struct s_chess_obj_set {
  ChessObj_Pieces_t pieces;
  ChessObj_Mvmt_Sel_t sels;
} ALIGN(sizeof(Obj_Attr_t)*4) ChessObj_Set_t;

typedef union u_chesspiece_roster {
  struct s_piece_roster_by_color {
    u16 black, white;
  } by_color;
  u32 all;
} ChessPiece_Roster_t;

#define PIECE_ROSTER_ID_TO_TEAM(id)\
  (id&PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT ? WHITE_FLAGBIT : BLACK_FLAGBIT)
#define PIECE_ROSTER_ID_TO_PIECE_IDX(id)\
  (BOARD_BACK_ROWS_INIT[(id&PIECE_ROSTER_ID_MASK)])

typedef struct s_chess_piece_data {
  ChessBoard_Idx_t location;
  Obj_Attr_t *obj_ctl;
  int roster_id;
} ALIGN(4) ChessPiece_Data_t;

typedef struct s_piece_tracker {
  ChessPiece_Roster_t roster;
  Obj_Attr_t *pieces_captured[2][CHESS_TEAM_PIECE_COUNT];
  int capcount[2];
  Graph_t *piece_graph;
} ChessPiece_Tracker_t;

typedef struct s_pgn_move {
  ChessBoard_Idx_t move[2];
  u16 roster_id;
  Move_Validation_Flag_e move_outcome;
} ALIGN(2) PGN_Move_t;
typedef struct s_pgn_round {
  PGN_Move_t moves[2];
} PGN_Round_t;

LL_DECL(PGN_Round_t, PGN_Round);
LL_DECL(ChessPiece_Data_t, PieceData);


typedef struct s_chess_context {
  ChessBoard_t board_data;
  ChessObj_Set_t obj_data;
  u32 whose_turn;
  ChessPiece_Tracker_t tracker;
  PGN_Round_LL_t move_hist;
  ChessBoard_Idx_t move_selections[2];
} __attribute__((aligned(4))) ChessGameCtx_t;

#define WHITE_SQUARE_CLR        0x679D
//#define BLACK_SQUARE_CLR        0x10A5
#define BLACK_SQUARE_CLR        0x2529
#define BLACK_PIECE_CLR         0x0421
#define WHITE_PIECE_CLR         0x4A97
#define WHITE_SEL_SQUARE_CLR    0x7F65
#define BLACK_SEL_SQUARE_CLR    0x5643

#define CHESS_BOARD_Y_OFS\
    ((SCREEN_HEIGHT - (Chess_sprites_Glyph_Height * CHESS_BOARD_ROW_COUNT))/2)

#define CHESS_BOARD_X_OFS\
    ((SCREEN_WIDTH - (Chess_sprites_Glyph_Width * CHESS_BOARD_FILE_COUNT))/2)
#define BOARD_IDX(bidx) bidx.coord.y][bidx.coord.x

#define GET_BOARD_AT_IDX(board, board_idx) (board[BOARD_IDX(board_idx)])


#define ABS(val, width) (((u##width)1<<(width-1))&(val) ? (~(val)+1) : val)


void ChessGameCtx_Init(ChessGameCtx_t *ctx);
void ChessBG_Init(void);
Move_Validation_Flag_e ChessBoard_ValidateMove(const ChessGameCtx_t *ctx);
Mvmt_Dir_e ChessBoard_MoveGetDir(const ChessBoard_Idx_t move[2]);


#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */
#endif  /* _CHESS_BOARD_H_ */
