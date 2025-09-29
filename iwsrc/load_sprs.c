#include <GBAdev_util_macros.h>
#include <GBAdev_types.h>
#include <GBAdev_memdef.h>
#include "chess_board.h"
#include "chess_sprites.h"
#include "chess_obj_sprites_data.h"

typedef union u_tile8_pbuf {
  u64 pbuf_by_row[8];
  u8 pbuf[8][8];
  Tile8_t tile;
} Tile8_Pbuf_t;



EWRAM_CODE static Tile8_Pbuf_t *Format_Chess_Sprite(Tile8_Pbuf_t *buf,
    const u8 *cspr, u16 fg_clr, u16 sel_bg_clr);


EWRAM_CODE Tile8_Pbuf_t *Format_Chess_Sprite(Tile8_Pbuf_t *buf,
    const u8 *cspr, u16 fg_clr, u16 sel_bg_clr) {
  ALIGN(8) static u64 spr64[CHESS_SPRITE_DIMS][CSPR_TILES_PER_DIM] = {0};
  u8 *spr = (u8*)spr64;
  u32 x,y=0;
  fg_clr&=0xFF;
  for (u8 curbit, curbyte; y < Chess_sprites_Glyph_Height; ++y) {
    for (curbit=0x80, curbyte=*cspr++, x=0; x < Chess_sprites_Glyph_Width; ++x) {
      spr[y*CHESS_SPRITE_DIMS+x] = curbit&curbyte ? fg_clr : sel_bg_clr;
      curbit>>=1;
      if (curbit)
        continue;
      curbit = 0x80;
      curbyte=*cspr++;
    }
  }
  y=0;
  for (u32 yy; y < CSPR_TILES_PER_DIM; ++y) {
    for (x=0; x < CSPR_TILES_PER_DIM; ++x, ++buf) {
      for (yy=0; yy < TILE8_DIMS; ++yy) {
        buf->pbuf_by_row[yy] = spr64[y*TILE8_DIMS + yy][x];
      }
    }
  }
  return buf;
}


EWRAM_CODE void Load_Chess_Sprites_8BPP(Tile8_t *dst, u16 fg_clr, u16 sel_bg_clr) {
  Tile8_Pbuf_t *curtile;
  u8 *csprs;
  curtile = (Tile8_Pbuf_t*)dst;
  csprs = (u8*)Chess_sprites_Glyph_Data;
  for (u32 ct = 0;
      ct < Chess_sprites_Glyph_Count;
      ++ct, csprs+=Chess_sprites_Glyph_Cell_Size)
    curtile = Format_Chess_Sprite(curtile, csprs, fg_clr, ct==EMPTY_IDX?sel_bg_clr:0);
}
