/** (C) 16 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#include <GBAdev_types.h>
#include <GBAdev_util_macros.h>
#include "chess_ai_types.h"
#include "chess_board.h"
#include "debug_io.h"
#include "chess_board_state_analysis.h"
struct s_board_state {
  ChessBoard_t board;
  GameState_t state;
  u64 zobrist;
  PieceState_Graph_t graph;
};
static EWRAM_CODE BOOL BoardState_ValidateAttackVector(
                                                const ChessBoard_Row_t *board,
                                                const ChessBoard_Idx_t *move,
                                                ChessPiece_e attacker,
                                                Mvmt_Dir_e attack_dir);

EWRAM_CODE BOOL BoardState_KingInCheck(const BoardState_t *board_state,
                                       u32 allied_king_id,
                                       u32 opp_ofs) {
  const PieceState_Graph_Vertex_t *vertices = board_state->graph.vertices;
  const ChessPiece_Roster_t roster = board_state->graph.roster;
  for (u32 j,jbase=0; CHESS_TEAM_PIECE_COUNT>jbase; ++jbase) {
    j=jbase|opp_ofs;
    if (!CHESS_ROSTER_PIECE_ALIVE(roster, j))
      continue;
    if (vertices[j].edges.all&(1<<allied_king_id)) {
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

/* Doesn't validate En Passent */
EWRAM_CODE BOOL BoardState_ValidateAttackVector(const ChessBoard_Row_t *board,
                                                const ChessBoard_Idx_t *move,
                                                ChessPiece_e attacker,
                                                Mvmt_Dir_e attack_dir) {
  u32 dx=move[1].arithmetic.x-move[0].arithmetic.x,
      dy=move[1].arithmetic.y-move[0].arithmetic.y;
  if (INVALID_MVMT_FLAGBIT&attack_dir)
    return FALSE;
  switch ((ChessPiece_e)(attacker&PIECE_IDX_MASK)) {
  case PAWN_IDX:
    if (!(DIAGONAL_MVMT_FLAGBIT&attack_dir))
      return FALSE;
    if (WHITE_FLAGBIT&attacker) {
      if (!(UP_FLAGBIT&attack_dir))
        return FALSE;
    } else {
      if (!(DOWN_FLAGBIT&attack_dir))
        return FALSE;
    }
    return (ABS(dy, 32)==1 && ABS(dx,32)==1);
  case BISHOP_IDX:
    if (!(DIAGONAL_MVMT_FLAGBIT&attack_dir))
      return FALSE;
    break;
  case ROOK_IDX:
    if ((KNIGHT_MVMT_FLAGBIT|DIAGONAL_MVMT_FLAGBIT)&attack_dir)
      return FALSE;
    assert((0!=(VER_MASK&attack_dir))^(0!=(HOR_MASK&attack_dir)));
    break;
  case KNIGHT_IDX:
    return 0!=(KNIGHT_MVMT_FLAGBIT&attack_dir);
  case QUEEN_IDX:
    if (KNIGHT_MVMT_FLAGBIT&attack_dir)
      return FALSE;
    if (DIAGONAL_MVMT_FLAGBIT&attack_dir) {
      assert(0!=(HOR_MASK&attack_dir) && 0!=(VER_MASK&attack_dir));
    } else {
      assert((0!=(VER_MASK&attack_dir))^(0!=(HOR_MASK&attack_dir)));
    }
    break;
  case KING_IDX:
    if (KNIGHT_MVMT_FLAGBIT&attack_dir)
      return FALSE;
    if (DIAGONAL_MVMT_FLAGBIT&attack_dir)
      return (1==ABS(dx, 32) && 1==ABS(dy, 32));
    else if (HOR_MASK&attack_dir)
      return 1==ABS(dx,32) && !dy;
    else if (VER_MASK&attack_dir)
      return 1==ABS(dy, 32) && !dx;
    else
      ensure(0, "Unexpected dir val. Attack dir = 0x%04X\n", attack_dir);
  default:
    assert(attacker!=EMPTY_IDX);
    assert(EMPTY_IDX > (attacker&PIECE_IDX_MASK) 
          && 0!=(attacker&PIECE_TEAM_MASK));
    exit(EXIT_FAILURE);
  }
  return ChessBoard_ValidateMoveClearance(board, move, attack_dir);
}




EWRAM_CODE int BoardState_Validate_CastleLegaility(
                                              const BoardState_t *board_state,
                                              ChessBoard_Idx_t dst) {
  const PieceState_Graph_Vertex_t *const 
         VERTS = board_state->graph.vertices;
  const u32 
    OPP_RID_OFS = ((board_state->state.side_to_move&WHITE_TO_MOVE_FLAGBIT)
                            ? 0
                            : PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT),
    ALLIED_KING_RID = KING|(OPP_RID_OFS^PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT),
    ALLIED_KING_RID_FLAGBIT = 1<<ALLIED_KING_RID;
  const i32 dx = FILE_C==dst.coord.x?-1:1;
  const ChessPiece_Roster_t roster = board_state->graph.roster;
  u32 abs_rid, rid=0;
  ChessBoard_Idx_t cur[2];
  const ChessBoard_File_e DST_FILE = dst.coord.x;
  Mvmt_Dir_e dir;
  ChessPiece_e cur_opp_piece; 
  const ChessPiece_e opp_team_flagbit = OPP_RID_OFS ? WHITE_FLAGBIT 
                                                    : BLACK_FLAGBIT;
  const ChessBoard_Idx_Compact_t ALLIED_KING_ORIGIN 
    = board_state->graph.vertices[ALLIED_KING_RID].location;
  ChessBoard_Idx_Compact_t opp_loc;
  for (const PieceState_Graph_Vertex_t *curvert;
         CHESS_TEAM_PIECE_COUNT>rid;
         ++rid) {
    abs_rid = OPP_RID_OFS|rid;
    if (!CHESS_ROSTER_PIECE_ALIVE(roster, abs_rid))
      continue;
    curvert = VERTS+abs_rid;
    if (curvert->edges.all&ALLIED_KING_RID_FLAGBIT)
      return BOARD_STATE_CASTLE_BLOCKED_BY_CHECK;
  }
  rid = 0;
  for (const ChessPiece_e (*BOARD)[CHESS_BOARD_FILE_COUNT] = board_state->board;
       CHESS_TEAM_PIECE_COUNT>rid;
       ++rid) {
    abs_rid = OPP_RID_OFS|rid;
    if (!CHESS_ROSTER_PIECE_ALIVE(roster, abs_rid))
      continue;
    opp_loc = VERTS[abs_rid].location;
    if (!(rid&PAWN0))
      cur_opp_piece = BOARD_BACK_ROWS_INIT[rid]|opp_team_flagbit;
    else
      cur_opp_piece = BOARD[BOARD_IDX(opp_loc)];
    cur[0] = BOARD_IDX_CONVERT(opp_loc, NORMAL_IDX_TYPE);
    cur[1] = BOARD_IDX_CONVERT(ALLIED_KING_ORIGIN, NORMAL_IDX_TYPE);
    for (ChessBoard_File_e curfl = (cur[1].coord.x+=dx);
         DST_FILE!=curfl;
         curfl = (cur[1].coord.x+=dx)) {
      dir = ChessBoard_MoveGetDir(cur);
      if (INVALID_MVMT_FLAGBIT&dir)
        continue;
      if (BoardState_ValidateAttackVector(BOARD, cur, cur_opp_piece, dir))
        return BOARD_STATE_CASTLE_BLOCKED_BY_PATH_ATTACK_PT;
    }
    assert(DST_FILE==cur[1].coord.x);
    dir = ChessBoard_MoveGetDir(cur);
    if (INVALID_MVMT_FLAGBIT&dir)
      continue;
    if (BoardState_ValidateAttackVector(BOARD, cur, cur_opp_piece, dir))
      return BOARD_STATE_CASTLE_CASTLE_BLOCKED_BY_REVEALED_CHECK;
  }
  return BOARD_STATE_CASTLE_OK;
}
