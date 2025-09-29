#include <GBAdev_types.h>
#include <GBAdev_memmap.h>
#include <GBAdev_memdef.h>
#include <GBAdev_functions.h>

u16 monochrome_inc(u16 col) {
  u16 r,g,b;
  r=1+(col&31);
  g=1+((col>>5)&31);
  b=1+((col>>10)&31);
  r&=31;
  g&=31;
  b&=31;
  return r|g<<5|b<<10;
}

int main(void) {
  REG_DPY_CNT = REG_FLAG(DPY_CNT, BG2)|REG_VALUE(DPY_CNT, MODE, 3);
  REG_IME = 0;
  REG_DPY_STAT |= REG_FLAG(DPY_STAT, VBL_IRQ);
  REG_ISR_MAIN = ISR_Handler_Basic;
  REG_IE |= 1<<IRQ_VBLANK;
  REG_IME = 1;
  for (u16 x,y,c;;) {
    for (y=0; y < M3_SCREEN_HEIGHT; ++y) {
      for (x=0; x < M3_SCREEN_WIDTH; ++x) {
        VRAM_M3[y*M3_SCREEN_HEIGHT+x] = (c=monochrome_inc(c));
        SUPERVISOR_CALL(0x05);
      }
    }
  }
}
