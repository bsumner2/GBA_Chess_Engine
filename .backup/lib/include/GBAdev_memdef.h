/** (C) 7 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _GBADEV_MEMDEF_
#define _GBADEV_MEMDEF_

#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

/* MEMORY REGIONS */
#define MEM_IO            0x04000000
#define MEM_PAL           0x05000000
#define MEM_VRAM          0x06000000
#define MEM_OAM           0x07000000
#define MEM_ROM           0x08000000
#define MEM_SRAM          0x0E000000

/* MEMORY SEGMENT SIZES */
#define M4_PAGE_SIZE      0x9600
#define M5_PAGE_SIZE      0xA000
#define SRAM_SIZE         0x10000

#define PAL_BG_SIZE                 0x0200
#define PAL_OBJ_SIZE                0x0200

#define VRAM_BG_SIZE                0x10000
#define VRAM_OBJ_SIZE               0x08000

#define M3_SIZE                     0x12C00
#define M4_SIZE                     0x09600
#define M5_SIZE                     0x0A000
#define BMP_PAGE_SIZE               0x0A000

/* GBA BMP mode screen buffer dims */
#define ABS_SCREEN_HEIGHT           160
#define ABS_SCREEN_WIDTH            240

#define M3_SCREEN_HEIGHT            160
#define M3_SCREEN_WIDTH             240

#define M4_SCREEN_HEIGHT            160
#define M4_SCREEN_WIDTH             240

#define M5_SCREEN_HEIGHT            128
#define M5_SCREEN_WIDTH             160

/* REG_KEY_STAT flagbits */
#define KEY_STAT_A_FLAGBIT          0x0001
#define KEY_STAT_B_FLAGBIT          0x0002
#define KEY_STAT_SEL_FLAGBIT        0x0004
#define KEY_STAT_START_FLAGBIT      0x0008
#define KEY_STAT_RIGHT_FLAGBIT      0x0010
#define KEY_STAT_LEFT_FLAGBIT       0x0020
#define KEY_STAT_UP_FLAGBIT         0x0040
#define KEY_STAT_DOWN_FLAGBIT       0x0080
#define KEY_STAT_R_FLAGBIT          0x0100
#define KEY_STAT_L_FLAGBIT          0x0200
#define KEY_STAT_KEYS_MASK          0x03FF

/* Alternative names for keypad status bits */
#define KEY_A                       KEY_STAT_A_FLAGBIT
#define KEY_B                       KEY_STAT_B_FLAGBIT
#define KEY_SEL                     KEY_STAT_SEL_FLAGBIT
#define KEY_START                   KEY_STAT_START_FLAGBIT
#define KEY_RIGHT                   KEY_STAT_RIGHT_FLAGBIT
#define KEY_LEFT                    KEY_STAT_LEFT_FLAGBIT
#define KEY_UP                      KEY_STAT_UP_FLAGBIT
#define KEY_DOWN                    KEY_STAT_DOWN_FLAGBIT
#define KEY_R                       KEY_STAT_R_FLAGBIT
#define KEY_L                       KEY_STAT_L_FLAGBIT

/* Display control reg bitfield flags and masks */
#define DPY_CNT_MODE_MASK           0x0007
#define DPY_CNT_GB_FLAGBIT          0x0008
#define DPY_CNT_PAGE_SEL_FLAGBIT    0x0010
#define DPY_CNT_OAM_HBL_FLAGBIT     0x0020
#define DPY_CNT_OBJ_1D_FLAGBIT      0x0040
#define DPY_CNT_BLANK_FLAGBIT       0x0080
#define DPY_CNT_BG0_FLAGBIT         0x0100
#define DPY_CNT_BG1_FLAGBIT         0x0200
#define DPY_CNT_BG2_FLAGBIT         0x0400
#define DPY_CNT_BG3_FLAGBIT         0x0800
#define DPY_CNT_OBJ_FLAGBIT         0x1000
#define DPY_CNT_WIN0_FLAGBIT        0x2000
#define DPY_CNT_WIN1_FLAGBIT        0x4000
#define DPY_CNT_WIN_OBJ_FLAGBIT     0x8000

/* Display status reg bitfield flag and masks bit */
#define DPY_STAT_IN_VBL_FLAGBIT       0x0001
#define DPY_STAT_IN_HBL_FLAGBIT       0x0002
#define DPY_STAT_IN_VCOUNT_FLAGBIT    0x0004
#define DPY_STAT_VBL_IRQ_FLAGBIT      0x0008
#define DPY_STAT_HBL_IRQ_FLAGBIT      0x0010
#define DPY_STAT_VCOUNT_IRQ_FLAGBIT   0x0020
#define DPY_STAT_VCOUNT_MASK          0xFF00

#define TILE8_DIMS                    8

#define DPY_CNT_MODE_SHAMT            0
#define DPY_STAT_VCOUNT_SHAMT         8

#define REG_FLAG(regname, flagname) \
  (regname##_##flagname##_FLAGBIT)
#define REG_VALUE(regname, flagname, value) ((value&regname##_##flagname##_MASK)<<regname##_##flagname##_SHAMT)

#define TRUE 1U
#define FALSE 0U

#define DPY_CNT_MODE_MODE3 REG_VALUE(DPY_CNT, MODE, 3)



#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _GBADEV_MEMDEF_ */
