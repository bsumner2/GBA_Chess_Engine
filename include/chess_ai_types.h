/** (C) 23 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _CHESS_AI_TYPES_
#define _CHESS_AI_TYPES_

#include "GBAdev_types.h"
#include "GBAdev_util_macros.h"
#include "chess_board.h"
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */
#define CASTLE_FLAG_COUNT 4

#define NO_VALID_EN_PASSENT_FILE ((u8)0xFFU)
#define VALID_FILE_MASK 7U
#define EN_PASSENT_POSSIBLE(state_ep_file)\
  (NO_VALID_EN_PASSENT_FILE!=state_ep_file)

#define WHITE_TO_MOVE_FLAGBIT ((u8)(WHITE_FLAGBIT>>8))
#define BLACK_TO_MOVE_FLAGBIT ((u8)(BLACK_FLAGBIT>>8))
#define MAX(a,b) ((a>b)?a:b)
#define MIN(a,b) ((a<b)?a:b)
/* PIECE_TEAM_MASK expands to (WHITE_FLAGBIT|BLACK_FLAGBIT), therefore
 * SIDE_TO_MOVE_MASK must expand to 
 * (WHITE_TO_MOVE_FLAGBIT|BLACK_TO_MOVE_FLAGBIT) 
 */
#define SIDE_TO_MOVE_MASK     (PIECE_TEAM_MASK>>8)
#define CONVERT_CTX_MOVE_FLAG(whose_turn) ((u8)((whose_turn)>>8))
#define CONVERT_BOARD_STATE_MOVE_FLAG(side_to_move) ((side_to_move)<<8)
#define IS_MAXIMIZING(side_to_move) (side_to_move & WHITE_TO_MOVE_FLAGBIT)

#define CASTLE_RIGHTS_WHITE_FLAGS_SHAMT 0
#define CASTLE_RIGHTS_BLACK_FLAGS_SHAMT 2
typedef enum e_castle_rights_flag {
  WK=1,
  WQ=2,
  BK=4,
  BQ=8,
  KINGSIDE_SHAMT_INVARIANT=1,
  QUEENSIDE_SHAMT_INVARIANT=2,
  WHITE_CASTLE_RIGHTS_MASK=3,
  BLACK_CASTLE_RIGHTS_MASK=12,
  ALL_CASTLE_RIGHTS_MASK=15
} PACKED CastleRightsFlag_e;
#define CASTLE_RIGHTS_SHAMT_INVARIANT 3

#define NON_FORFEITED_CASTLE_RIGHTS(board_state)\
  (u16)(~((board_state)->state.castle_rights_forfeiture)&ALL_CASTLE_RIGHTS_MASK)

typedef struct s_board_state BoardState_t;
#define INVALID_IDX_COMPACT_RAW 0xFF
#define INVALID_IDX_COMPACT\
  ((ChessBoard_Idx_Compact_t){.raw=INVALID_IDX_COMPACT_RAW})
typedef union u_chess_board_idx_compact {
  struct s_chess_board_idx_compact {
    u8 x:4,y:4;
  } PACKED coord;
  u8 raw;
} ChessBoard_Idx_Compact_t;

typedef struct s_game_state {
  u16 fullmove_number;
  u8 side_to_move;
  u8 castle_rights;
  u8 ep_file;
  u8 halfmove_clock;
  u8 castle_rights_forfeiture;
} ALIGN(4) GameState_t;

typedef union u_piece_adj_bitfield {
  struct s_white_adj_bitfield {
    u16 attacking;
    u16 defending;
  } white_adjs;

  struct s_black_adj_bitfield {
    u16 defending;
    u16 attacking;
  } black_adjs;

  struct s_piece_roster_by_color team_invariant;
  u32 all;
} PieceAdjacencyFields_t;

typedef struct s_piece_graph_vertex {
  PieceAdjacencyFields_t edges;
  u8 defending_count, attacking_count, total_edge_count;
  ChessBoard_Idx_Compact_t location;
} PieceState_Graph_Vertex_t;


typedef struct s_piece_graph {
  PieceState_Graph_Vertex_t vertices[CHESS_TOTAL_PIECE_COUNT];
  u8 vertex_hashmap[CHESS_BOARD_ROW_COUNT][CHESS_BOARD_FILE_COUNT];
  ChessPiece_Roster_t roster;
} PieceState_Graph_t;

typedef struct s_chess_move_iteration {
  ChessBoard_Idx_t dst;
  ChessPiece_e promotion_flag;
  Move_Validation_Flag_e special_flags;
} ChessMoveIteration_t;

#define NORMAL_IDX_TYPE ChessBoard_Idx_t
#define COMPACT_IDX_TYPE ChessBoard_Idx_Compact_t
#define BOARD_IDX_CONVERT(normal_idx, _convert_to)\
  ((_convert_to) {\
    .coord = {\
      .x=(u8)(normal_idx.coord.x&7),\
      .y=(u8)(normal_idx.coord.y&7)\
     }\
   })
#define PIECE_GRAPH_EMPTY_HASHENT 0xFFU
#define PIECE_GRAPH_HASHMAP_INITIALIZER_WORD\
  (PIECE_GRAPH_EMPTY_HASHENT\
   |PIECE_GRAPH_EMPTY_HASHENT<<8\
   |PIECE_GRAPH_EMPTY_HASHENT<<16\
   |PIECE_GRAPH_EMPTY_HASHENT<<24)



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _CHESS_AI_TYPES_ */
