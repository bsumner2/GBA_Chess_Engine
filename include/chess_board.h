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
#define BLACK_ROSTER_ID(piece_roster_id) (piece_roster_id)
#define WHITE_ROSTER_ID(piece_roster_id) (PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT|piece_roster_id)


#define FILE(file) FILE_##file
#define ROW(row) ROW##row

typedef enum e_mvmt_dir {
  UP_FLAGBIT                = 0x0002,
  DOWN_FLAGBIT              = 0x0001,
  LEFT_FLAGBIT              = 0x0008,
  RIGHT_FLAGBIT             = 0x0004,
  HOR_MASK                  = 0x000C,
  VER_MASK                  = 0x0003,
  DIAGONAL_MVMT_FLAGBIT     = 0x0010,
  KNIGHT_MVMT_FLAGBIT       = 0x0020,
  MVMT_WIDE_FLAGBIT         = 0x0040,
  INVALID_MVMT_FLAGBIT      = 0x0080
} Mvmt_Dir_e;

/* I know this is redundant, but I don't care */
typedef enum e_knight_mvmt_dir {
  KNIGHT_MVMT_UP_FLAGBIT    = UP_FLAGBIT,
  KNIGHT_MVMT_DOWN_FLAGBIT  = DOWN_FLAGBIT,
  KNIGHT_MVMT_LEFT_FLAGBIT  = LEFT_FLAGBIT,
  KNIGHT_MVMT_RIGHT_FLAGBIT = RIGHT_FLAGBIT,
  KNIGHT_MVMT_TALL_FLAGBIT  = 0x4000,
  KNIGHT_MVMT_WIDE_FLAGBIT  = 0x8000,
  KNIGHT_MVMT_DIM_MASK      = 0xC000
} Knight_Mvmt_Dir_e;

#define INVALID_IDX_RAW_VAL 0xFFFFFFFFFFFFFFFFULL
#define INVALID_IDX ((ChessBoard_Idx_t){.raw=INVALID_IDX_RAW_VAL})
#define OUT_OF_BOUNDS_IDX_MASK 0xFFFFFFF8FFFFFFF8ULL
typedef union u_board_idx {
  struct s_chess_coord {
    ChessBoard_File_e x;
    ChessBoard_Row_e y;
  } coord;
  struct s_chess_coord_arithmetic {
    int x, y;
  } arithmetic;
  u64 raw;
} ChessBoard_Idx_t;

#define CHESS_BOARD_FILE_COUNT 8
#define CHESS_BOARD_ROW_COUNT  8
#define CHESS_BOARD_SQUARE_COUNT (CHESS_BOARD_FILE_COUNT*CHESS_BOARD_ROW_COUNT)
#define CHESS_TEAM_PIECE_COUNT (CHESS_BOARD_FILE_COUNT*2)
#define CHESS_TOTAL_PIECE_COUNT (CHESS_TEAM_PIECE_COUNT*2)

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


#define INVALID_PGN_MOVE \
    (PGN_Move_t){ \
        .move = { { .raw = INVALID_IDX_RAW_VAL }, \
                  { .raw = INVALID_IDX_RAW_VAL } }, \
        .move_outcome = MOVE_UNSUCCESSFUL, \
        .roster_id    = 0xFFFFU, \
        .promotion    = 0xFFFFFFFFU \
    }

typedef struct s_pgn_move {
  ChessBoard_Idx_t move[2];
  u16 roster_id;
  Move_Validation_Flag_e move_outcome;
  ChessPiece_e promotion;
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
// OTHER COLOR OPTS FOR BLACK SQUARE: 0x10A5
#define BLACK_SQUARE_CLR        0x2529
// OTHER COLOR OPTS FOR BLACK PIECE: 0x0421, 0x2D4D, 0x250B, 0x39B0
#define BLACK_PIECE_CLR         0x250B
#define WHITE_PIECE_CLR         0x4A97
#define VALID_SEL_SQUARE_CLR    0x7F65
#define INVALID_SEL_SQUARE_CLR  0x31B8

#define CHESS_BOARD_Y_OFS\
    ((SCREEN_HEIGHT - (Chess_sprites_Glyph_Height * CHESS_BOARD_ROW_COUNT))/2)

#define CHESS_BOARD_X_OFS\
    ((SCREEN_WIDTH - (Chess_sprites_Glyph_Width * CHESS_BOARD_FILE_COUNT))/2)
#define BOARD_IDX(bidx) bidx.coord.y][bidx.coord.x

#define GET_BOARD_AT_IDX(board, board_idx) (board[BOARD_IDX(board_idx)])
#define CHESS_ROSTER_PIECE_ALIVE(roster, piece_roster_id)\
  (roster.all&(1<<(piece_roster_id)))


#define ABS(val, width) (((u##width)1<<(width-1))&(val) ? (~(val)+1) : val)


void ChessGameCtx_Init(ChessGameCtx_t *ctx);
void ChessBG_Init(void);
Move_Validation_Flag_e ChessBoard_ValidateMove(const ChessGameCtx_t *ctx);
Mvmt_Dir_e ChessBoard_MoveGetDir(const ChessBoard_Idx_t move[2]);
BOOL ChessBoard_FindNextObstruction(const ChessBoard_t board_data,
                                    const ChessBoard_Idx_t *start,
                                    ChessBoard_Idx_t *return_obstruction_idx,
                                    Mvmt_Dir_e dir);
Knight_Mvmt_Dir_e ChessBoard_KnightMoveGetDir(const ChessBoard_Idx_t *mv,
                                              Mvmt_Dir_e dir);
int ChessBoard_KingInCheck(ChessPiece_Data_t **return_pieces,
                           const ChessPiece_Tracker_t *tracker,
                           u32 team);
BOOL ChessBoard_ValidateKingMoveEvadesCheck(
                                     const ChessGameCtx_t *ctx,
                                     const ChessPiece_Data_t *checking_pieces,
                                     u32 checking_piece_ct,
                                     Move_Validation_Flag_e move_result);
BOOL ChessBoard_KingMove_EntersCheck(const ChessBoard_t board_data,
                                     const ChessBoard_Idx_t move[2],
                                     const ChessPiece_Tracker_t *tracker);

BOOL ChessBoard_ValidateMoveClearance(const ChessBoard_t board_data,
                                      const ChessBoard_Idx_t move[2],
                                      Mvmt_Dir_e dir);

Move_Validation_Flag_e ChessBoard_ValidateMoveLegality(
                                     const ChessBoard_t board_data,
                                     const ChessBoard_Idx_t move[2],
                                     const ChessPiece_Tracker_t *tracker,
                                     const PGN_Round_LL_t *mvmt_ll);

void ChessGameCtx_Close(ChessGameCtx_t *ctx);

#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */
#endif  /* _CHESS_BOARD_H_ */
