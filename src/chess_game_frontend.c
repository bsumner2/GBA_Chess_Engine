#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <GBAdev_memmap.h>
#include <string.h>
#include "GBAdev_functions.h"
#include "GBAdev_memdef.h"
#include "chess_gameloop.h"
#include "key_status.h"
#include "mode3_io.h"
#include "subpixel.h"
#include "chess_game_frontend.h"
#define TOSTRSTR(l) #l
#define TOSTR(m) TOSTRSTR(m)
static const char *MODE_SEL_PROMPTS[4] = {
  "2-player mode",
  "1-player X CPU",
  "CPU X 1-player",
  "CPU X CPU",
};

static const size_t MODE_SEL_PROMPT_LENS[4] = {
  LSTRLEN("2-player mode"),
  LSTRLEN("1-player X CPU"),
  LSTRLEN("CPU X 1-player"),
  LSTRLEN("CPU X CPU")
};
static void M3_SelScreen(int sel, int prev);
void M3_SelScreen(int sel, int prev) {
  assert((SEL_MASK&sel)==sel);
  if (0 > prev) {
    for (int i = 0; SEL_COUNT > i; ++i) {
      if ((const int)sel==i) {
        mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[i]),
                     ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*i,
                     SELECT_CLR,
                     "%s", // Pass it as a format arg to get rid of warning
                     MODE_SEL_PROMPTS[i]);
      } else {
        mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[i]),
                     ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*i,
                     NORMAL_CLR,
                     "%s",  // Pass it as a format arg to get rid of warning
                     MODE_SEL_PROMPTS[i]);
      }
    }
    return;
  }
  assert((SEL_MASK&prev)==prev);
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[sel]),
               ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*sel,
               SELECT_CLR,
               "%s",  // Pass it as a format arg to get rid of warning
               MODE_SEL_PROMPTS[sel]);

  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(MODE_SEL_PROMPT_LENS[prev]),
               ((SCREEN_WIDTH - SubPixel_Glyph_Height) / 5)*prev,
               NORMAL_CLR,
               "%s",  // Pass it as a format arg to get rid of warning
               MODE_SEL_PROMPTS[prev]);
}

int M3FE_SelGamemode(void) {
  int prev=0, cur=0;
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);
  M3_SelScreen(0, -1);
  for (IRQ_Sync(IRQ_FLAG(KEYPAD)); !KEY_STROKE(A); IRQ_Sync(IRQ_FLAG(KEYPAD))) {
    if (KEY_STROKE(UP)) {
      --cur;
    } else if (KEY_STROKE(DOWN)) {
      ++cur;
    } else {
      continue;
    }
    cur&=SEL_MASK;
    M3_SelScreen(cur, prev);
    prev = cur;
  }

  Fast_Memset32(VRAM_M3,
                0,
                sizeof(u16)*M3_SCREEN_HEIGHT*M3_SCREEN_WIDTH/sizeof(WORD));

  switch ((GameModeSelection_e)cur) {
    case GAME_MODE_2PLAYER:
      return 0;
      break;
    case GAME_MODE_1PLAYER_V_CPU:
    case GAME_MODE_CPU_V_1PLAYER:
      return PIECE_TEAM_MASK^(cur<<12);
      break;
    case GAME_MODE_CPU_V_CPU:
      return PIECE_TEAM_MASK;
      break;
    default:
      assert(SEL_MASK&cur);
  }

}

#if 0
typedef struct s_m3_bmp {
  u32 x, y, width, height;
  u16 color;
} BMP_Rect_t;

extern BOOL Mode3_Draw_Rect(const BMP_Rect_t *r);
const char *M3FE_SavePrompt(char *buf, u32 buflen) {
  const u32 XORIGIN = SUBPIXEL_FONT_TEXT_HPOS_CENTERED(buflen), 
            YORIGIN = SUBPIXEL_FONT_TEXT_VPOS_CENTERED;
  u32 cursor_pos = 0, prev_pos;
  assert(32>=buflen);
  assert(NULL!=buf && 0<buflen);
  M3_CLR_SCREEN();
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("Save Game as PGN?")),
      SUBPIXEL_FONT_TEXT_VPOS_CENTERED-SubPixel_Glyph_Height/2, 0x10A5,
      "Save Game as PGN?");
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("[A] = Yes [B] = No")),
              SUBPIXEL_FONT_TEXT_VPOS_CENTERED+SubPixel_Glyph_Height/2, AFFIRMITIVE_CLR,
              "[A]\x1b[0x10A5] = Yes \x1b[" TOSTR(NEGATIVE_CLR) "][B]\x1b[0x10A5] = No"); 
  while (IRQ_Sync(IRQ_FLAG(KEYPAD)), !KEY_STROKE(A))
    if (KEY_STROKE(B)) return NULL;
  if (KEY_STROKE(B))
    return NULL;
  M3_CLR_SCREEN();
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("Enter PGN file name:")),
      8,
      0x10A5, "Enter PGN file name:");
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("[Controls]:")),
        M3_SCREEN_HEIGHT-8-SubPixel_Glyph_Height*4, NORMAL_CLR, "[Controls]:");
  mode3_printf(SUBPIXEL_FONT_TEXT_HPOS_CENTERED(LSTRLEN("[^/v] = Cycle chars @ cursor")),
        M3_SCREEN_HEIGHT-8-SubPixel_Glyph_Height*3, SELECT_CLR,
        "[^/v]\x1b[" TOSTR(NORMAL_CLR) "] = Cycle chars @ cursor\n"
        "\x1b[0x44E4][A/B]\x1b[" TOSTR(NORMAL_CLR) "] = Advance/Backspace cursor\n"
        "\x1b[" TOSTR(AFFIRMITIVE_CLR) "][START]\x1b[" TOSTR(NORMAL_CLR) "] = Confirm name");
  {
    BMP_Rect_t bufrect = {
      .x=XORIGIN,
      .y=YORIGIN,
      .width=SubPixel_Glyph_Width*(buflen-1), 
      .height=SubPixel_Glyph_Height,
      .color = NORMAL_CLR
    };
    Mode3_Draw_Rect(&bufrect);
  }
  if (0!=(buflen/sizeof(WORD))) {
    Fast_Memset32(buf, 0, buflen/sizeof(WORD));
  }
  if (buflen&(sizeof(WORD)-1)) {
    const u32 REMAINDER = buflen&(sizeof(WORD)-1);
    char *buf_remainder = &buf[buflen-REMAINDER];
    for (u32 i = 0; REMAINDER>i; ++i)
      buf_remainder[i] = '\0';
  }
  buf[cursor_pos] = ' ';

  mode3_putchar(' ', XORIGIN, YORIGIN, SELECT_CLR);
  for (IRQ_Sync(IRQ_FLAG(KEYPAD)); 
       !KEY_STROKE(START) && prev_pos!=cursor_pos;
       IRQ_Sync(IRQ_FLAG(KEYPAD))) {
    if (!(KEY_STAT_KEYS_MASK&KEY_CURR))
      continue;
    if (KEY_STROKE(UP, DOWN)) {
      if (KEY_PRESSED(UP) && KEY_PRESSED(DOWN))
        continue;
      prev_pos = cursor_pos;
      if (KEY_STROKE(UP)) {
        if (buf[cursor_pos]>='\x7F')
          buf[cursor_pos] = ' ';
        else
          ++buf[cursor_pos];
      } else {
        if (buf[cursor_pos]<=' ')
          buf[cursor_pos] = '\x7F';
        else
          --buf[cursor_pos];
      }
      mode3_putchar(buf[cursor_pos], XORIGIN+cursor_pos*SubPixel_Glyph_Width,
                    YORIGIN, SELECT_CLR);
      continue;
    }
    if (KEY_STROKE(A,B)) {
      if (KEY_PRESSED(A) && KEY_PRESSED(B))
        continue;
      prev_pos = cursor_pos;
      if (KEY_STROKE(A)) {
        ++cursor_pos;
        if (buflen<=cursor_pos) {
          cursor_pos = 0;
        }
      } else {
        if (0==cursor_pos) {
          cursor_pos = buflen-1;
        } else {
          --cursor_pos;
        }
      }
      if (!buf[cursor_pos])
        buf[cursor_pos] = ' ';
    }
    mode3_putchar(buf[prev_pos], XORIGIN+cursor_pos*SubPixel_Glyph_Width,
                  YORIGIN, NORMAL_CLR);
    mode3_putchar(buf[cursor_pos], XORIGIN+cursor_pos*SubPixel_Glyph_Width,
                  YORIGIN, SELECT_CLR);
     
  }
  for (cursor_pos = buflen-1; 
       isspace(buf[cursor_pos]) || !isprint(buf[cursor_pos]); --cursor_pos)
    continue;
  ++cursor_pos;
  assert(cursor_pos <= buflen);
  memset(&buf[cursor_pos], 0, buflen-cursor_pos+1);
  return buf;
}
#endif
