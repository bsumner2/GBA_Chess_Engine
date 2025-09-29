/** (C) 7 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _GBADEV_MEMMAP_
#define _GBADEV_MEMMAP_
#include "GBAdev_memdef.h"
#include "GBAdev_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#define REG(type, addr) (*((volatile type*) (addr)))

#define REG_IFBIOS                REG(u16, MEM_IO - 0x0008)
#define REG_ISR_MAIN              (*((void (**)(void))(MEM_IO - 0x0004)))

#define REG_DPY_CNT	              REG(u32, MEM_IO)
#define REG_DPY_STAT	            REG(u16, MEM_IO + 0x0004)
#define REG_DPY_VCOUNT	          REG(u16, MEM_IO + 0x0006)

#define REG_BG0_CNT               REG(u16, MEM_IO + 0x0008)
#define REG_BG1_CNT               REG(u16, MEM_IO + 0x000A)
#define REG_BG2_CNT               REG(u16, MEM_IO + 0x000C)
#define REG_BG3_CNT               REG(u16, MEM_IO + 0x000E)

#define REG_BG0_HOFS              REG(u16, MEM_IO + 0x0010)
#define REG_BG0_VOFS              REG(u16, MEM_IO + 0x0012)

#define REG_BG1_HOFS              REG(u16, MEM_IO + 0x0014)
#define REG_BG1_VOFS              REG(u16, MEM_IO + 0x0016)

#define REG_BG2_HOFS              REG(u16, MEM_IO + 0x0018)
#define REG_BG2_VOFS              REG(u16, MEM_IO + 0x001A)

#define REG_BG3_HOFS              REG(u16, MEM_IO + 0x001C)
#define REG_BG3_VOFS              REG(u16, MEM_IO + 0x001E)

#define REG_BG2_PA                REG(u16, MEM_IO + 0x0020)
#define REG_BG2_PB                REG(u16, MEM_IO + 0x0022)
#define REG_BG2_PC                REG(u16, MEM_IO + 0x0024)
#define REG_BG2_PD                REG(u16, MEM_IO + 0x0026)
#define REG_BG2_X                 REG(u32, MEM_IO + 0x0028)
#define REG_BG2_Y                 REG(u32, MEM_IO + 0x002C)

#define REG_BG3_PA                REG(u16, MEM_IO + 0x0030)
#define REG_BG3_PB                REG(u16, MEM_IO + 0x0032)
#define REG_BG3_PC                REG(u16, MEM_IO + 0x0034)
#define REG_BG3_PD                REG(u16, MEM_IO + 0x0036)
#define REG_BG3_X                 REG(u32, MEM_IO + 0x0038)
#define REG_BG3_Y                 REG(u32, MEM_IO + 0x003C)

#define REG_WIN0_HDIMS            REG(u16, MEM_IO + 0x0040)
#define REG_WIN1_HDIMS            REG(u16, MEM_IO + 0x0042)

#define REG_WIN0_VDIMS            REG(u16, MEM_IO + 0x0044)
#define REG_WIN1_VDIMS            REG(u16, MEM_IO + 0x0046)

#define REG_WIN_IN                REG(u16, MEM_IO + 0x0048)
#define REG_WIN_OUT               REG(u16, MEM_IO + 0x004A)
#define REG_MOSAIC_SZ             REG(u16, MEM_IO + 0x004C)

#define REG_BLEND_CNT             REG(u16, MEM_IO + 0x0050)
#define REG_BLEND_ALPHA           REG(u16, MEM_IO + 0x0052)
#define REG_BLEND_FADE            REG(u16, MEM_IO + 0x0054)
#define REG_BLEND_Y               REG(u16, MEM_IO + 0x0054)

#define REG_SND1_SWEEP            REG(u16, MEM_IO + 0x0060)
#define REG_SND1_CNT              REG(u16, MEM_IO + 0x0062)
#define REG_SND1_FREQ             REG(u16, MEM_IO + 0x0064)

#define REG_SND2_CNT              REG(u16, MEM_IO + 0x0068)
#define REG_SND2_FREQ             REG(u16, MEM_IO + 0x006C)

#define REG_SND3_SEL              REG(u16, MEM_IO + 0x0070)
#define REG_SND3_CNT              REG(u16, MEM_IO + 0x0072)
#define REG_SND3_FREQ             REG(u16, MEM_IO + 0x0074)

#define REG_SND4_CNT              REG(u16, MEM_IO + 0x0078)
#define REG_SND4_FREQ             REG(u16, MEM_IO + 0x007C)

#define REG_SND_CNT			          REG(u32, MEM_IO + 0x0080)
#define REG_SND_DMG_CNT		        REG(u16, MEM_IO + 0x0080)
#define REG_SND_DSOUND_CNT 	 	    REG(u16, MEM_IO + 0x0082)
#define REG_SND_STAT			        REG(u16, MEM_IO + 0x0084)
#define REG_SND_BIAS			        REG(u16, MEM_IO + 0x0088)

#define REG_WAVE_RAM		          ((volatile u32*) (MEM_IO + 0x0090))

#define REG_WAVE_RAM0			 	 	    REG(u32, MEM_IO + 0x0090)
#define REG_WAVE_RAM1			 	 	    REG(u32, MEM_IO + 0x0094)
#define REG_WAVE_RAM2			 	 	    REG(u32, MEM_IO + 0x0098)
#define REG_WAVE_RAM3			 	 	    REG(u32, MEM_IO + 0x009C)

#define REG_FIFO_A				 	 	    REG(u32, MEM_IO + 0x00A0)
#define REG_FIFO_B				 	 	    REG(u32, MEM_IO + 0x00A4)

#define REG_DMA                   ((volatile DMA_Handle_t*) (MEM_IO + 0x00B0))

#define REG_TM                    ((volatile Timer_Handle_t*) (MEM_IO + 0x0100))

#define REG_SIO_CNT               REG(u16, MEM_IO + 0x0128)

#define REG_SIO_DATA              ((volatile u32*) (MEM_IO + 0x0120))
#define REG_SIO_DATA32            REG(u32, MEM_IO + 0x0120)
#define REG_SIO_DATA8             REG(u16, MEM_IO + 0x012A)

#define REG_SIO_MULTI             ((volatile u16*) (MEM_IO + 0x0120))
#define REG_SIO_MULTI0            REG(u16, MEM_IO + 0x0120)
#define REG_SIO_MULTI1            REG(u16, MEM_IO + 0x0122)
#define REG_SIO_MULTI2            REG(u16, MEM_IO + 0x0124)
#define REG_SIO_MULTI3            REG(u16, MEM_IO + 0x0126)

#define REG_SIO_MULTI_RECV        REG(u16, MEM_IO + 0x0120)
#define REG_SIO_MULTI_SEND        REG(u16, MEM_IO + 0x012A)

#define REG_KEY_STAT              REG(u16, MEM_IO + 0x0130)
#define REG_KEY_CNT               REG(u16, MEM_IO + 0x0132)

#define REG_RCNT                  REG(u16, MEM_IO + 0x0134)

#define REG_JOY_CNT               REG(u16, MEM_IO + 0x0140)
#define REG_JOY_RECV              REG(u16, MEM_IO + 0x0150)
#define REG_JOY_TRANS             REG(u32, MEM_IO + 0x0154)
#define REG_JOY_STAT              REG(u16, MEM_IO + 0x0158)

#define REG_IE                    REG(u16, MEM_IO + 0x0200)
#define REG_IF                    REG(u16, MEM_IO + 0x0202)
#define REG_WAIT_CNT              REG(u16, MEM_IO + 0x0204)
#define REG_IME                   REG(u16, MEM_IO + 0x0208)
/* Unsure about this one */
#define REG_PAUSE                 REG(u16, MEM_IO + 0x0300)



#define OAM_ATTR                  ((Obj_Attr_t*) (MEM_OAM))
#define OAM_AFFINE                ((Obj_Affine_Transform_t*) (MEM_OAM))


#define SRAM                      ((u8*) (MEM_SRAM))


#define VRAM_M3                   ((u16*) (MEM_VRAM))

#define VRAM_M4_P0                ((u8*)  (MEM_VRAM))
#define VRAM_M4_P1                ((u8*)  (MEM_VRAM + M4_PAGE_SIZE))

#define VRAM_M5_P0                ((u16*) (MEM_VRAM))
#define VRAM_M5_P1                ((u16*) (MEM_VRAM + M5_PAGE_SIZE))

#define TILE4_MEM                 ((Tile4_Block_t*) (MEM_VRAM))
#define TILE8_MEM                 ((Tile8_Block_t*) (MEM_VRAM))


#define PAL_MEM_BG                ((u16*) (MEM_PAL))
#define PAL_MEM_OBJ               ((u16*) (MEM_PAL + PAL_BG_SIZE))


#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _GBADEV_MEMMAP_ */
