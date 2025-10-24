/* Header for zobrist table generated using /dev/random for rng */
#ifndef ZOBRIST_H
#define ZOBRIST_H

#ifdef __cplusplus
extern "C" {
#endif  /* C++ name mangler guard */

#include <stdint.h>
typedef enum e_zobrist_piece_id {
  WHITE_PAWN_ZID=0,
  WHITE_BISHOP_ZID,
  WHITE_ROOK_ZID,
  WHITE_KNIGHT_ZID,
  WHITE_QUEEN_ZID,
  WHITE_KING_ZID,
  BLACK_PAWN_ZID,
  BLACK_BISHOP_ZID,
  BLACK_ROOK_ZID,
  BLACK_KNIGHT_ZID,
  BLACK_QUEEN_ZID,
  BLACK_KING_ZID
} ZobristPieceID_t;

#define ZID_FROM_BOARD_PIECE_DATA(piece)\
  (EMPTY_IDX==piece\
      ? 0xFFFFFFFFUL\
      : (PIECE_IDX_MASK&piece)\
            + ((WHITE_FLAGBIT&piece) ? 0 : 6))

#define ZOBRIST_SIZE 793
#define ZOBRIST_PIECE_ID_COUNT 12
#define ZOBRIST_CASTLE_ID_COUNT 16
#define ZOBRIST_EN_PASSENT_ID_COUNT 8

#define ZOBRIST_CASTLE_ID_MASK (ZOBRIST_CASTLE_ID_COUNT-1)
#define ZOBRIST_EN_PASSENT_ID_MASK (ZOBRIST_EN_PASSENT_ID_COUNT-1)

#define BOARD_ZKEY_ENTS(table)\
    (*(const u64(*)[CHESS_BOARD_ROW_COUNT][CHESS_BOARD_FILE_COUNT]\
       [ZOBRIST_PIECE_ID_COUNT]) (table))
#define SIDE_TO_MOVE_ZKEY_ENT(table) (table[CHESS_BOARD_ROW_COUNT*CHESS_BOARD_FILE_COUNT*ZOBRIST_PIECE_ID_COUNT])
#define CASTLE_ZKEY_ENTS(table)\
  (*(const u64(*)[ZOBRIST_CASTLE_ID_COUNT])\
    (table + CHESS_BOARD_ROW_COUNT*CHESS_BOARD_FILE_COUNT\
                *ZOBRIST_PIECE_ID_COUNT\
           + 1))
#define EN_PASSENT_ZKEY_ENTS(table)\
  (*(const u64(*)[ZOBRIST_EN_PASSENT_ID_COUNT])\
    (table + CHESS_BOARD_ROW_COUNT*CHESS_BOARD_FILE_COUNT\
                *ZOBRIST_PIECE_ID_COUNT\
           + 1 + ZOBRIST_CASTLE_ID_COUNT))
extern const unsigned long long int zobrist_table[ZOBRIST_SIZE];

#ifdef __cplusplus
}
#endif  /* C++ name mangler guard closer */

#endif /* ZOBRIST_H */
