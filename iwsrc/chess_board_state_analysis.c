/** (C) 16 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#include "chess_board.h"
#include "chess_board_state.h"
#include "chess_board_state_analysis.h"
struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};

EWRAM_CODE BOOL BoardState_KingInCheck(const BoardState_t *board_state,
                                       u32 allied_king_id,
                                       u32 opp_ofs) {
  const PieceState_Graph_Vertex_t *vertices = board_state->graph.vertices;
  for (u32 jbase=0; CHESS_TEAM_PIECE_COUNT>jbase; ++jbase) {
    if (vertices[jbase|opp_ofs].edges.all&(1<<allied_king_id)) {
      return TRUE;
    }
  }
  return FALSE;
}

EWRAM_CODE Mvmt_Dir_e BoardState_PiecePinDirection(
                                               const BoardState_t *board_state,
                                               ChessPiece_e piece_id) {
  ChessBoard_Idx_t move[2];
  PieceState_Graph_Vertex_t cur_opp_vert;
  
  const PieceState_Graph_Vertex_t *vertices = board_state->graph.vertices;
  const ChessPiece_e (*board_data)[CHESS_BOARD_ROW_COUNT] = board_state->board;
  const u32 ALLIED_OFS = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT&piece_id,
            OPP_OFS = PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT&~piece_id,
            PIECE_FLAGBIT = (1<<piece_id);
  Mvmt_Dir_e piece_to_king, opp_to_piece;
  
  ChessBoard_Idx_t piece_loc, king_loc, opp_loc;
  piece_loc = BOARD_IDX_CONVERT(vertices[piece_id].location, NORMAL_IDX_TYPE);
  king_loc = BOARD_IDX_CONVERT(vertices[ALLIED_OFS|KING].location, NORMAL_IDX_TYPE);
  move[0] = piece_loc;
  move[1] = king_loc;
  piece_to_king = ChessBoard_MoveGetDir(move);
  if ((KNIGHT_MVMT_FLAGBIT|INVALID_MVMT_FLAGBIT)&piece_to_king) {
    return INVALID_MVMT_FLAGBIT;
  }
  assert(ChessBoard_FindNextObstruction(board_data, 
        &piece_loc, &move[0], piece_to_king));
  if (king_loc.raw!=move[0].raw) {
    // If the next obstruction isnt king, theres something between self and
    // king that will block check in lieu of this piece moving
    return INVALID_MVMT_FLAGBIT;
  }
  for (u32 ibase=0; CHESS_TEAM_PIECE_COUNT>ibase;++ibase) {
    cur_opp_vert = vertices[ibase|OPP_OFS];
    opp_loc = BOARD_IDX_CONVERT(cur_opp_vert.location, NORMAL_IDX_TYPE);
    switch ((ChessPiece_e)(PIECE_IDX_MASK&board_data[BOARD_IDX(opp_loc)])) {
    case PAWN_IDX:
    case KNIGHT_IDX:
    case KING_IDX:
      continue;
    case BISHOP_IDX:
    case ROOK_IDX:
    case QUEEN_IDX:
      break;
    default:
      assert(0);
      break;
    }
    if (!(cur_opp_vert.edges.all&PIECE_FLAGBIT))
      continue;
    move[0] = opp_loc;
    move[1] = piece_loc;
    opp_to_piece = ChessBoard_MoveGetDir(move);
    if (opp_to_piece==piece_to_king) {
      // Since transitive rule upheld across board idxs w.r.t directions, 
      // if we know king is to <direction X> of piece,
      // and we know piece is to <direction X> of opp,
      // then by transitive rule, we can be certain that
      // king is to <direction X> of opp.
      return piece_to_king;
    }
  }
  return INVALID_MVMT_FLAGBIT;
}

