/** (C) 8 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _GBADEV_TYPES_
#define _GBADEV_TYPES_


#include "GBAdev_util_macros.h"

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */
typedef unsigned char           u8;
typedef unsigned short int      u16;
typedef unsigned long int       u32;
typedef unsigned long long int  u64;

typedef char                    i8;
typedef short int               i16;
typedef long int                i32;
typedef long long int           i64;

typedef _Bool                   BOOL;

typedef unsigned long int       WORD;

typedef void (*IRQ_Callback_t)(void);
typedef enum e_irq_index {
  IRQ_VBLANK=0,
  IRQ_HBLANK,
  IRQ_VCOUNT,
  IRQ_TIMER0,
  IRQ_TIMER1,
  IRQ_TIMER2,
  IRQ_TIMER3,
  IRQ_SERIAL,
  IRQ_DMA0,
  IRQ_DMA1,
  IRQ_DMA2,
  IRQ_DMA3,
  IRQ_KEYPAD,
  IRQ_GAMEPAK,
  IRQ_MAX
} IRQ_Index_t;
/** Register types */

typedef struct s_dpy_cnt_fields {
  u32 mode : 3;
  BOOL gb_gbc_mode : 1;
  BOOL page_sel : 1;
  BOOL oam_enable_hblank : 1;
  BOOL obj_1d_sprite_map : 1;
  BOOL force_screen_blank : 1;
  BOOL bg0_enable : 1;
  BOOL bg1_enable : 1;
  BOOL bg2_enable : 1;
  BOOL bg3_enable : 1;
  BOOL obj_enable : 1;
  BOOL win0_enable : 1;
  BOOL win1_enable : 1;
  BOOL win_obj_enable : 1;
} ALIGN(2) PACKED Dpy_Cnt_Fields_t;

typedef struct s_dpy_stat_fields {
  BOOL in_vblank : 1;
  BOOL in_hblank : 1;
  BOOL in_vcount : 1;
  BOOL vblank_irq_enable : 1;
  BOOL hblank_irq_enable : 1;
  BOOL vcount_irq_enable : 1;
  u32 vcount_trigger_val : 8;
} ALIGN(2) PACKED Dpy_Stat_Fields_t;

typedef struct s_key_stat_fields {
  BOOL A : 1;
  BOOL B : 1;
  BOOL SEL : 1;
  BOOL START : 1;
  BOOL RIGHT : 1;
  BOOL LEFT : 1;
  BOOL UP : 1;
  BOOL DOWN : 1;
  BOOL R : 1;
  BOOL L : 1;
  u32 IGNORE_PAD0 : 6;
} ALIGN(2) PACKED Key_Stat_Fields_t;

typedef struct s_tm_cnt_fields {
  u32 freq:2;
  BOOL cascade_mode:1;
  u32 IGNORE_PAD0:3;
  BOOL interrupt_upon_completion:1;
  BOOL enable:1;
  BOOL IGNORE_PAD1;
} ALIGN(2) PACKED Timer_CNT_Fields_t;

typedef struct s_dma_cnt_fields {
  u32 ntransfers : 16;
  u32 IGNORE_PAD0 : 5;
  u32 dest_adjustment : 2;
  u32 src_adjustment : 2;
  BOOL repeat : 1;
  BOOL word_sized_chunks : 1;
  BOOL IGNORE_PAD1 : 1;
  u32 timing_mode : 2;
  BOOL interrupt_upon_completion : 1;
  BOOL enable : 1;
} ALIGN(2) PACKED DMA_CNT_Fields_t;

typedef struct s_ieif_fields {
  BOOL vblank : 1;
  BOOL hblank : 1;
  BOOL vcount : 1;
  u32 timer : 4;
  BOOL com : 1;
  u32 dma : 4;
  BOOL keypad : 1;
  BOOL gamepak : 1;
  u32 IGNORE_PAD0 : 2;
} ALIGN(2) PACKED IE_Fields_t, IF_Fields_t;

 
typedef struct s_dma_handle {
  const void *src;
  void *dst;
  union { u32 raw; DMA_CNT_Fields_t fields; } cnt_reg;
} DMA_Handle_t;

typedef struct s_tm_handle {
  u16 data;
  union { u16 raw; Timer_CNT_Fields_t fields; } cnt_reg;
} Timer_Handle_t;

typedef union u_obj_attr0 {
  u16 raw;
  struct {
    u32 y             : 8;
    BOOL affine_mode  : 1;
    BOOL disable      : 1;
    u32 mode          : 2;
    BOOL mosaic       : 1;
    BOOL pal_8bpp     : 1;
    u32 shape         : 2;
  } ALIGN(2) PACKED regular;
  struct {
    u32 y             : 8;
    BOOL affine_mode  : 1;
    BOOL double_size  : 1;
    u32 mode          : 2;
    BOOL mosaic       : 1;
    BOOL pal_8bpp     : 1;
    u32 shape         : 2;
  } ALIGN(2) PACKED affine;
  struct {
    u32 y             : 8;
    BOOL affine_mode  : 1;
    BOOL IGNORE_PAD0  : 1;
    u32 mode          : 2;
    BOOL mosaic       : 1;
    BOOL pal_8bpp     : 1;
    u32 shape         : 2;
  } ALIGN(2) PACKED shared;
} Obj_Attr_Fields0_t;

typedef union u_obj_attr1 {
  u16 raw;
  struct {
    u32 x : 9;
    u32 IGNORE_PAD0 : 3;
    BOOL hflip : 1;
    BOOL vflip : 1;
    u32 obj_size : 2;
  } ALIGN(2) PACKED regular;
  struct {
    u32 x : 9;
    u32 affine_idx : 5;
    u32 obj_size : 2;
  } ALIGN(2) PACKED affine;
  struct {
    u32 x : 9;
    u32 IGNORE_PAD0 : 5;
    u32 obj_size : 2;
  } ALIGN(2) PACKED shared;
} Obj_Attr_Fields1_t;

typedef struct s_obj_attr2 {
  u32 sprite_idx : 10;
  u32 priority : 2;
  u32 pal16_bank : 4;
} ALIGN(2) PACKED Obj_Attr_Fields2_t;



typedef struct s_obj_attr {
  Obj_Attr_Fields0_t attr0;
  Obj_Attr_Fields1_t attr1;
  Obj_Attr_Fields2_t attr2;
  u16 IGNORE_PAD;
} ALIGN(4) Obj_Attr_t;

typedef struct s_obj_affine_transform {
  u16 IGNORE_PAD0[3], 
      pa, 
      IGNORE_PAD1[3],
      pb,
      IGNORE_PAD2[3],
      pc,
      IGNORE_PAD3[3],
      pd;
} ALIGN(4) Obj_Affine_Transform_t;

typedef struct s_spr_tile_4bpp {
  u32 data[8];
} Tile4_t;

typedef struct s_spr_tile_8bpp {
  u32 data[16];
} Tile8_t;

typedef Tile4_t Tile4_Block_t[512];
typedef Tile8_t Tile8_Block_t[256];



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _GBADEV_TYPES_ */
