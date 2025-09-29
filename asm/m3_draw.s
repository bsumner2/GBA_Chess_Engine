@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ Copyright Burt O Sumner 2025 @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@       Rights Reserved        @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@   Usage, copy, edit, and     @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ redistribution permitted,    @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ but only if this copyright   @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ stays visible, and untouched @@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#include "asm_macros.inc"

#define LD_RECT(rs, ...) LDM rs, { __VA_ARGS__ }
#define ST_RECT(rd, ...) STM rd, { __VA_ARGS__ }
    .extern Fast_Memset32

    .section .iwram,"ax", %progbits
    .arm
    .align 2
    .type Mode3_Draw_RectARM %function
Mode3_Draw_RectARM:
    /*  r0:= rect::struct Rect*::struct Rect {WORD x, y, width, height; HWORD color; }* */
    PUSH { r4-r10, fp, lr }
    MOV fp, sp
    LDRH r1, [r0, #16]
    /* r9:= (color&0xFFFF)|((color&0xFFFF)<<16) */
    MVN r9, #0
    AND r9, r1, r9, LSR #16
    ORR r9, r9, r9, LSL #16
    


    LD_RECT(r0, r0, r1, r6, r7)  /* r0-r3:= r.x, r.y, r.width, r.height respectively */
    
    /* Save r.x and r.y outside of scratch registers, and also if either of them
     * are negative, return -1 */
    MOVS r4, r0
    MOVPLS r5, r1
    BMI .Lmode3_draw_rect_ARM_invalid_arg

    /* if r.width <= 0 || r.height <= 0 return -1 */
    CMP r6, #0
    CMPGT r7, #0
    BLE .Lmode3_draw_rect_ARM_invalid_arg

    /* if r.x > SCREEN_WIDTH || r.width + r.x > SCREEN_WIDTH return -1 */
    RSBS r0, r0, #SCRN_WIDTH
    SUBCSS r0, r0, r6
    BCC .Lmode3_draw_rect_ARM_invalid_arg

    /* if r.y > SCREEN_HEIGHT || r.height + r.y > SCREEN_WIDTH return -1 */
    RSBS r1, r1, #SCRN_HEIGHT
    SUBCSS r1, r1, r7
    BCC .Lmode3_draw_rect_ARM_invalid_arg
    
    /* r8:= r.y*(2^4) - r.y = r.y*16 - r.y = r.y*15 */
    RSB r8, r5, r5, LSL #4
    
    /* r8:= r.y*15*32 = r.y*480 = r.y*240*2 = r.y*SCREEN_WIDTH*2
     *      = (r.y Scanlines)*(SCREEN_WIDTH pixels/scanline) * 2B/pixel
     *      = r.y*SCREEN_WIDTH*2 Bytes to offset from base coord (0,0) VRAM addr
     *      = VRAM offset of r.y's scanline (final unit: Bytes)*/
    MOV r8, r8, LSL #5
 
    /* r8:= r.y*SCRN_WIDTH*2 + r.x*2
     *      = VRAM offset of r.y's scanline 
     *              + ((r.x px) * 2B/px:= r.x*2 Bytes of offset from
     *                  scanline to r.x)
     *      = VRAM offset of r's upper-left corner */
    ADD r8, r8, r4, LSL #1

    /* NOTE: Regarding the validity of the following instruction and its 
     * equivalence to an addition operation as portrayed in the accompanying 
     * comment:
     *      We can just OR them instead of adding them because the first 24 bits of
     *      VRAM's base address are zero, and the range of addresses of VRAM, as a 
     *      whole, is only [0x06000000, 0x06012C00) aka [0x06000000, 0x06012BFF].
     *      We can safely OR them because the offset range [0x000000, 0x012C00)
     *      aka [0x000000, 0x012BFF] has no offset value that will overlap with 
     *      any of the bits of VRAM's base addr: effectively making any addition of
     *      VRAM_BASE + <offset> <=> VRAM_BASE | <offset>) */
    
    /* r8:= Address of r's top-left corner in VRAM 
     *          = VRAM_BASE + VRAM offset of r's top-left corner */
    ORR r8, r8, #VRAM_BASE  /* VRAM address of r's upper-left corner */

.Lmode3_draw_rect_ARM_loop:
        /* IF r.x&2 != 0 then we gotta write the first pixel before calling
         * memset for the rest of the words of rect pixel data to write to this 
         * scanline, so that memset has a word-aligned src address param'd */
        TST r4, #1
        SUBNE r10, r6, #1  /* r10:= width-1 = rwidth */
        STRNEH r9, [r8]
        ADDNE r0, r8, #2  /* r0:= addr of remainder of line = rline */
        MOVEQ r0, r8  /* r0:= addr of remainder of line = rline */
        MOVEQ r10, r6  /* r10:= width = rwidth */

        /* r2:= r10/2 = rwidth pixels * 2B/pixel / 4B/word 
         *         = (rwidth*2 B)/(4 words) 
         *         = (rwidth/2) words
         * This will be words param passed to memset */
        MOVS r2, r10, LSR #1
        TSTEQ r10, #1  /* if remainder of line is 0 words long, check if 
                        * remainder is also 0 hwords... */
        BEQ .Lmode3_draw_rect_ARM_loop_continue  /* ... and if so continue. */
        
        TEQ r2, #0  /* If remainder words is zero, then we know that hwords def 
                     * isn't, so in this scenario, all that's left is one hword,
                     * so write that and continue. */
        STREQH r9, [r0]
        BEQ .Lmode3_draw_rect_ARM_loop_continue
        /* now that we've dealt with those cases, all that's left is memset
         * remainder words, and also if remainder words leaves 1 remainder 
         * hword to write afterward */
        MOV r1, r9  /* r1:= (color&0xFFFF)|(color&0xFFFF<<16) */
        BL Fast_Memset32
        TST r10, #1  /* If rwidth%2 != 0 then we know memset left one last halfword to write */
        EORNE r10, r10, #1
        MOVNE r10, r10, LSL #1
        STRNEH r9, [r8, r10]
        

.Lmode3_draw_rect_ARM_loop_continue:
        SUBS r7, r7, #1
        ADDNE r8, r8, #(SCRN_WIDTH*2)
        BNE .Lmode3_draw_rect_ARM_loop
        


    MOV sp, fp
    MOV r0, #1
    POP { r4-r10, fp, lr }
    BX lr

.Lmode3_draw_rect_ARM_invalid_arg:
    MOV sp, fp
    MVN r0, #0
    POP { r4-r10, fp, lr }
    BX lr
    .size Mode3_Draw_RectARM, .-Mode3_Draw_RectARM

    .text
    .thumb_func
    .align 2
    .global Mode3_ClearScreen
    .type Mode3_ClearScreen %function
Mode3_ClearScreen:
    /* r0:= color */
    MOV r1, r0
    LSL r1, r1, #16
    ORR r1, r1, r0  /* (color<<16)|color */

    MOV r0, #0xC0
    LSL r0, r0, #19  /* r0:= 0xC0<<(3+16) = 0x600<<16 = 0x600*(0x10^4)
                      *         = 0x06000000 = &VRAM */
    /* NOTE: So to fill screen with Mode3_ClearScreen#ARG0 (color), we need to
     * fill 240*160 pixels' worth of VRAM space. Mode 3 interprets VRAM as 16BPP
     * (ABGR) BMP, so:
     *      -) bytewise: we have to write 76800 bytes:
     *              (160scanlines*240px/scanline)*2bytes/px 
     *                      = 38400px*2bytes/px
     *                      = 76800B
     *      -) Wordwise: 19200words 
     *              76800B/(4B/word) = 19200words
     *  To simplify:
     *      160scanlns*(240px/scanlns)*(2B/px)/(4B/word) 
     *              = 160sl*(240px/sl)*(1word/2px) = 160sl*(120words/sl)
     *              = (160*(128-8))(sl*words/sl) = (160*128 - 160*8)words
     *              = (160*2^7 - 160*2^3)words = ((160<<7) - (160<<3))words
     *              = 19200words
     * We need to do the math this roundabout way due to the limitations of both
     * ARMv4t in general, but, more specifically, the limitations of THUMB mode.
     */
    MOV r2, #SCRN_WIDTH  /* r2:= SCRN_WIDTH = 160 */
    LSL r2, r2, #3  /* r2:= SCRN_WIDTH*(2^3) */
    MOV r3, r2
    LSL r3, r3, #4  /* r3:= SCRN_WIDTH*(2^3)*(2^4) = 2^7 */
    SUB r2, r3, r2
    LDR r3, =Fast_Memset32
    BX r3
    .size Mode3_ClearScreen, .-Mode3_ClearScreen


    .thumb_func
    .align 2
    .global Mode3_Draw_Rect
    .type Mode3_Draw_Rect %function
Mode3_Draw_Rect:
    LDR r2, =Mode3_Draw_RectARM
    BX r2
    .size Mode3_Draw_Rect, .-Mode3_Draw_Rect
