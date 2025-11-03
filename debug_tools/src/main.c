#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define ALIGN(x) __attribute__ (( aligned(x) ))
#define PACKED __attribute__ (( packed ))

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
} PACKED ChessPiece_e;
static_assert(sizeof(ChessPiece_e)==2);

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

typedef enum e_move_validation_flags {
  MOVE_EN_PASSENT=0x8000,
  MOVE_CASTLE_QUEENSIDE=0x4000,
  MOVE_CASTLE_KINGSIDE=0x2000,
  MOVE_PAWN_TWO_SQUARE=0x1000,
  MOVE_CASTLE_MOVE_FLAGS_MASK=0x6000,
  MOVE_SPECIAL_MOVE_FLAGS_MASK=0xF000,
  MOVE_CAPTURE=0x0800,

  MOVE_SUCCESSFUL=1,
  MOVE_UNSUCCESSFUL=0,
} PACKED Move_Validation_Flag_e;

static_assert(sizeof(Move_Validation_Flag_e)==2);

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

typedef struct s_chess_board_idx {
  ChessBoard_File_e x;
  ChessBoard_Row_e y;
} ChessBoard_Idx_t;


typedef struct s_pgn_move {
  ChessBoard_Idx_t move[2];
  unsigned short roster_id;
  Move_Validation_Flag_e move_outcome;
  ChessPiece_e promotion;
} ALIGN(8) PGN_Move_t;


typedef struct s_pgn_round {
  PGN_Move_t moves[2];
} PGN_Round_t;

typedef enum e_chess_piece_roster_id {
  PIECE_ROSTER_ABS_ID_MASK=31, PIECE_ROSTER_ID_MASK=15,  PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT=16,
  ROOK0=0, KNIGHT0, BISHOP0, QUEEN, KING, BISHOP1, KNIGHT1, ROOK1,
  PAWN0, PAWN1, PAWN2, PAWN3, PAWN4, PAWN5, PAWN6, PAWN7
} ChessPiece_Roster_Id_e;
#define BLACK_ROSTER_ID(piece_roster_id) (piece_roster_id)
#define WHITE_ROSTER_ID(piece_roster_id) (PIECE_ROSTER_ID_WHITE_TEAM_FLAGBIT|piece_roster_id)

#define MIN(a,b) (a<b ? a : b)

static_assert(sizeof(enum e_file)==4);
static_assert(sizeof(enum e_row)==4);
#define perrf(fmt, ...) fprintf(stderr, "\x1b[1;31m[Error]: \x1b[0m" fmt, __VA_ARGS__)
#define perr(s) fputs("\x1b[1;31m[Error]: \x1b[0m" s, stderr)


char *strdupe(const char *str) {
  char *ret;
  int len = strlen(str);
  ret = malloc(len+1);
  strncpy(ret, str, len);
  ret[len] = '\0';
  return ret;
}
char *GetMoveString(PGN_Move_t *move) {
  Move_Validation_Flag_e flag = move->move_outcome;
  if (flag&MOVE_CASTLE_MOVE_FLAGS_MASK) {
    return flag&MOVE_CASTLE_KINGSIDE ? strdupe("O-O") : strdupe("O-O-O");
  }
  char *str;
  if (flag&MOVE_CAPTURE) {
    str = malloc(sizeof("FR Captures FR"));
    str[0] = 'A'+move->move[0].x;
    str[1] = '8'-move->move[0].y;
    str[2] = ' ';
    strncpy(&str[3], "Captures ", 10);
    str[sizeof("FR Captures FR")-1] = '\0';
    str[sizeof("FR Captures FR")-2] = '8'-move->move[1].y;
    str[sizeof("FR Captures FR")-3] = 'A'+move->move[1].x;
  } else {
    str = malloc(sizeof("FR -> FR"));
    str[0] = 'A'+move->move[0].x;
    str[1] = '8'-move->move[0].y;
    str[2] = ' ';
    strncpy(&str[3], "-> ", 4);
    str[sizeof("FR -> FR")-1] = '\0';
    str[sizeof("FR -> FR")-2] = '8'-move->move[1].y;
    str[sizeof("FR -> FR")-3] = 'A'+move->move[1].x;
  }
  return str;


  
}

const char *PieceType_ToString(ChessPiece_e pc) {
  switch (pc) {
  case BLACK_BISHOP: return "BLACK_BISHOP";
  case BLACK_ROOK: return "BLACK_ROOK";
  case BLACK_KNIGHT: return "BLACK_KNIGHT";
  case BLACK_QUEEN: return "BLACK_QUEEN";
  case BLACK_KING: return "BLACK_KING";
  case WHITE_BISHOP: return "WHITE_BISHOP";
  case WHITE_ROOK: return "WHITE_ROOK";
  case WHITE_KNIGHT: return "WHITE_KNIGHT";
  case WHITE_QUEEN: return "WHITE_QUEEN";
  case WHITE_KING: return "WHITE_KING";
  case WHITE_FLAGBIT: return "WHITE_PAWN";
  case BLACK_FLAGBIT: return "BLACK PAWN";
  default: return "INVALID";
    break;
  }
}


const char *PieceRID_ToString(ChessPiece_Roster_Id_e rid) {
  switch (rid) {
  case ROOK0: return "ROOK0";
  case KNIGHT0: return "KNIGHT0";
  case BISHOP0: return "BISHOP0";
  case QUEEN: return "QUEEN";
  case KING: return "KING";
  case BISHOP1: return "BISHOP1";
  case KNIGHT1: return "KNIGHT1";
  case ROOK1: return "ROOK1";
  case PAWN0: return "PAWN0";
  case PAWN1: return "PAWN1";
  case PAWN2: return "PAWN2";
  case PAWN3: return "PAWN3";
  case PAWN4: return "PAWN4";
  case PAWN5: return "PAWN5";
  case PAWN6: return "PAWN6";
  case PAWN7: return "PAWN7";
  default: return "INVALID";
    break;
  }
}

int main(int argc, char *argv[]) {
  if (argc!=2) {
    perrf("Invalid args. Usage:\n\t\x1b[1;34m%s\x1b[22;36m <save file to "
        "deconstruct>\x1b[0m", argv[0]);
    return 1;
  }
  {
    int len = strlen(argv[1]), valid = 0;
    do {
      if (4 > len)
        break;
        
      if (strncmp(&argv[1][len-4], ".sav", 4))
        break;
      valid = 1;
    } while (0);
    if (!valid) {
      perrf("Invalid arg, \x1b[1;31m%s\x1b[0m, is not a \x1b[1;36m.sav file"
          "\x1b[0m.\n", argv[1]);
      return 1;
    }
  }

  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    perrf("Failed to open save file \x1b[1;31m%s\x1b[0m.\n", argv[1]);
    return 1;
  }
  
  uint32_t rounds, last_turn;
  
  fread(&rounds, 4, 1, fp);
  fread(&last_turn, 4, 1, fp);
  if (0 > (signed)rounds || ((last_turn&PIECE_TEAM_MASK)!=last_turn) || !last_turn) {
    perrf("Invalid save file header. Save file header implies negative round "
        "count and/or invalid last turn flag:\n\t"
        "Round count: \x1b[1;36m0x%08X\x1b[0m\n\t"
        "Last Turn Flag: \x1b[1;36m0x%04X\x1b[0m\n",
        rounds, last_turn);
    return 1;
  }
  PGN_Round_t *pgn = malloc(rounds);
  fread(pgn, rounds, 1, fp);
  fclose(fp);
  fp = NULL;
  ChessPiece_e piece_types[32] = {
    ROOK_IDX, KNIGHT_IDX, BISHOP_IDX, QUEEN_IDX, KING_IDX, BISHOP_IDX,
    KNIGHT_IDX, ROOK_IDX,
    PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX,
    PAWN_IDX,
    ROOK_IDX, KNIGHT_IDX, BISHOP_IDX, QUEEN_IDX, KING_IDX, BISHOP_IDX,
    KNIGHT_IDX, ROOK_IDX,
    PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX, PAWN_IDX,
    PAWN_IDX,
  };
  rounds/=sizeof(PGN_Round_t);
  // gotta process last round differently, so decrement rounds count by one.
  if (WHITE_FLAGBIT==last_turn)
    --rounds;
  PGN_Move_t *curround_moves;
  char *curmove_name;
  for (uint32_t i = 0; rounds > i; ++i) {
    curround_moves = pgn[i].moves;
    curmove_name = GetMoveString(curround_moves);
    if (curround_moves[0].promotion!=0 && (PAWN0&curround_moves[0].roster_id)!=0) {
      piece_types[WHITE_ROSTER_ID(curround_moves[0].roster_id)] = curround_moves[0].promotion;
    }
    printf("\x1b[1;33mRound %d:\x1b[0m\n\t"
        "Moving Piece ID: \x1b[1;34m0x%04X\t=\tWHITE %s\x1b[0m\n\t"
        "Moving Piece Type: \x1b[1;35m%s\x1b[0m\n\t"
        "Move: %s\n\t",
        i+1,
        WHITE_ROSTER_ID(curround_moves[0].roster_id),
        PieceRID_ToString(curround_moves[0].roster_id),
        PieceType_ToString(WHITE_FLAGBIT|piece_types[WHITE_ROSTER_ID(curround_moves[0].roster_id)]),
        curmove_name);
    free(curmove_name);
    curmove_name = GetMoveString(curround_moves+1);
    if (curround_moves[1].promotion!=0 && (PAWN0&curround_moves[1].roster_id)!=0) {
      piece_types[BLACK_ROSTER_ID(curround_moves[1].roster_id)] = curround_moves[1].promotion;
    }
    printf("Moving Piece ID: \x1b[1;34m0x%04X\t=\tWHITE %s\x1b[0m\n\t"
        "Moving Piece Type: \x1b[1;35m%s\x1b[0m\n\t"
        "Move: %s\n",
        BLACK_ROSTER_ID(curround_moves[1].roster_id),
        PieceRID_ToString(curround_moves[1].roster_id),
        PieceType_ToString(BLACK_FLAGBIT|piece_types[BLACK_ROSTER_ID(curround_moves[1].roster_id)]),
        curmove_name);
    free(curmove_name);
    curmove_name = NULL;
  }
  if (WHITE_FLAGBIT==last_turn) {
    PGN_Move_t *move = pgn[rounds].moves;
    curmove_name = GetMoveString(move);
    if (curround_moves[0].promotion!=0 && (PAWN0&curround_moves[0].roster_id)!=0) {
      piece_types[WHITE_ROSTER_ID(curround_moves[0].roster_id)] = curround_moves[0].promotion;
    }

    printf("\x1b[1;33mRound %d:\x1b[0m\n\t"
        "Moving Piece ID: \x1b[1;34m0x%04X\t=\tWHITE %s\x1b[0m\n\t"
        "Moving Piece Type: \x1b[1;35m%s\x1b[0m\n\t"
        "Move: %s\n\t",
        rounds+1,
        WHITE_ROSTER_ID(curround_moves[0].roster_id),
        PieceRID_ToString(curround_moves[0].roster_id),
        PieceType_ToString(WHITE_FLAGBIT|piece_types[WHITE_ROSTER_ID(curround_moves[0].roster_id)]),
        curmove_name);
    curmove_name = NULL;
  }

  free(pgn);
  return 0;

  


  

}
