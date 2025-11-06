/******************************************************************************\
|*************************** Author: Burt O Sumner ****************************|
|******** Copyright 2025 (C) Burt O Sumner | All Rights Reserved **************|
\******************************************************************************/
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GBAdev_types.h>
#include <GBAdev_memdef.h>
#include <GBAdev_memmap.h>
#include <GBAdev_functions.h>
#include "mode3_io.h"
#include "subpixel.h"

typedef struct s_m3_bmp {
  u32 x, y, width, height;
  u16 color;
} BMP_Rect_t;

extern BOOL Mode3_Draw_Rect(const BMP_Rect_t *r);

#define Subpixel_Glyph_PxPair_Per_Row (SubPixel_Glyph_Width/2)
 
static int ipow(int base, u32 power) {
  if (power&1) return base*ipow(base*base, power>>1);
  else return power ? ipow(base*base, power>>1) : 1;
}


/*
 * @brief Use after encountering escape char, ASCII(27). Advance the pointer to
 * so that it's pointing to the char immediately following the esc char.
 * */
static u16 parse_color(char **buf_ptr, bool *return_errflag) {
  char *str = *buf_ptr;
  u16 ret = 0;
  char tmp;
  int len = 0, i = 0;
  *return_errflag = 1;
  if (*str++ != '[')
    return 0;
  if (strncmp(str++, "0x", 2))
    return 0;
  while ((tmp = *++str) && tmp != ']') {
    if (!isxdigit(tmp) || ++len > 4)
      return 0;
  }
  if (!tmp || !len)
    return 0;
  if (!str[1]) {
    *buf_ptr = ++str;
    return 0;  // Color won't matter anyway if EOS reached.
  }
  for (tmp = tolower(*(str - 1 - (i = 0))); i < len;
      tmp = tolower(*(str - 1 - ++i))) {
    tmp = (tmp > '9' ? 10 + tmp - 'a' : tmp - '0');
    ret += tmp*ipow(16, i);
  }
  *buf_ptr = ++str;
  *return_errflag = 0;
  return ret;
}

PRINTF_LIKE(4,5) int mode3_printf(int x,int y,u16 bg_clr,
                                  const char*__restrict fmt, ...) {
  BMP_Rect_t tabrect = {
    .x=0,.y=0,.width=SubPixel_Glyph_Width*4, .height=SubPixel_Glyph_Height
  };
  va_list args;
  const SubPixel_Pair_t *g_row;
  u16 *vbuf, clr_idx;
  va_start(args, fmt);
  int len = vsnprintf(NULL, 0, fmt, args), idx;
  va_end(args);
  va_start(args, fmt);

  char *buf = calloc(len+1,sizeof(char));
  if (NULL==buf) {
    for (u32 x=0,y=0,i = 0; (sizeof("Malloc returned NULL. Heap ruined.")-1)>i; ++i) {
      mode3_putchar("Malloc returned NULL. Heap ruined."[i], x,y,0x1484);
      x+=SubPixel_Glyph_Width;
      if (M3_SCREEN_WIDTH>=x+SubPixel_Glyph_Width)
        continue;
      x=0;
      y+=SubPixel_Glyph_Height;
    }
    exit(1);

  }
  va_start(args, fmt);
  vsnprintf(buf, len+1, fmt, args);
  if (buf[len]) buf[len] = '\0';
  va_end(args);

  char *pstr=buf, cur;
  int x_pos=x, y_pos=y, ret;
  bool err_flag;

  while ((cur=*pstr++)) {
    ret = pstr-&buf[0] - 1;
    if (cur < ' ') {
      if (cur == '\n') {
        x_pos = x;
        y_pos += SubPixel_Glyph_Height;
        if ((y_pos + SubPixel_Glyph_Height) > M3_SCREEN_HEIGHT) {
          free(buf);
          return ret;
        }
      } else if (cur == '\t') {
          tabrect.x=x_pos;
          tabrect.y=y_pos;
          tabrect.color = bg_clr;
          Mode3_Draw_Rect(&tabrect);


        x_pos += SubPixel_Glyph_Width*4;
      } else if (cur == '\x1b') {
        bg_clr = parse_color(&pstr, &err_flag);
        if (err_flag) {
          free(buf);
          return ret;
        }
      }
      continue;
    }

    idx = cur - ' ';
    if (idx >= SubPixel_Glyph_Count) {
      continue;
    }

    if ((x_pos + SubPixel_Glyph_Width) > M3_SCREEN_WIDTH) {
      x_pos = x;
      y_pos += SubPixel_Glyph_Height;
      if ((y+SubPixel_Glyph_Height) > M3_SCREEN_HEIGHT) {
        free(buf);
        return ret;
      }
      
    }

    g_row = (SubPixel_Pair_t*) (&SubPixel_Glyph_Data[idx*8]);
    vbuf = (y_pos*M3_SCREEN_WIDTH) + x_pos + VRAM_M3;

    for (int i = 0; i < SubPixel_Glyph_Height; g_row+=2, 
                                               vbuf+=M3_SCREEN_WIDTH,
                                               ++i)
      for (int j = 0; j < Subpixel_Glyph_PxPair_Per_Row; ++j) {

        if ((clr_idx=g_row[j].l)) {
          vbuf[j*2] = SubPixel_Pal[clr_idx];
        } else {
          vbuf[j*2] = bg_clr;
        }

        if ((clr_idx=g_row[j].r)) {
          vbuf[j*2 + 1] = SubPixel_Pal[clr_idx];
        } else {
          vbuf[j*2+1] = bg_clr;
        }
      }

    x_pos += SubPixel_Glyph_Width;
  }
  ret = pstr - &buf[0] - 1;
  free(buf);
  return ret;
}

void mode3_putchar(int c, int x, int y, u16 bg_color) {
  u16 *vrbuf, clr_idx;
  c -= ' ';
  if ( c < 0 || c >= SubPixel_Glyph_Count)
    return;
  if (x < 0 || y < 0)
    return;
  if (x+4>M3_SCREEN_WIDTH || y+8>M3_SCREEN_HEIGHT)
    return;
  
  SubPixel_Pair_t *g_row = (SubPixel_Pair_t*)(&SubPixel_Glyph_Data[c*8]);
  vrbuf = x+y*M3_SCREEN_WIDTH+VRAM_M3;
  if (bg_color&0x8000) {
    for (int i = 0; i < SubPixel_Glyph_Height; ++i) {
      for (int j = 0; j < 2; ++j) {
        if ((clr_idx=g_row[j].l)) {
          vrbuf[j*2] = SubPixel_Pal[clr_idx];
        }

        if ((clr_idx=g_row[j].r)) {
          vrbuf[j*2 + 1] = SubPixel_Pal[clr_idx];
        }
      }
      vrbuf+=M3_SCREEN_WIDTH;
      g_row+=2;
    }
  
  }
  
  for (int i = 0; i < SubPixel_Glyph_Height; ++i) {
    for (int j = 0; j < 2; ++j) {
      if ((clr_idx=g_row[j].l)) {
        vrbuf[j*2] = SubPixel_Pal[clr_idx];
      } else {
        vrbuf[j*2] = bg_color;
      }
 
      if ((clr_idx=g_row[j].r)) {
        vrbuf[j*2 + 1] = SubPixel_Pal[clr_idx];
      } else {
        vrbuf[j*2+1] = bg_color;
      }
    }
    vrbuf+=M3_SCREEN_WIDTH;
    g_row+=2;
  }
}
