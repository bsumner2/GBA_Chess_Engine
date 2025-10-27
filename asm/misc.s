    .extern ISR_Handler_Basic
    .section .bss
    .align 2
    .global KEY_CURR
    .global KEY_PREV
    .type KEY_CURR %object
    .type KEY_PREV %object
KEY_CURR:
    .zero 2
    .size KEY_CURR, 2
KEY_PREV:
    .zero 2
    .size KEY_PREV, 2
    
    .section .text
    .thumb_func
    .align 2
    .global IRQ_Sync
    .type IRQ_Sync %function
IRQ_Sync:
    MOV r1, r0
    MOV r0, #1
    SVC 0x04
    BX lr
    .size IRQ_Sync, .-IRQ_Sync

    .thumb_func
    .align 2
    .global SRAM_Fill
    .type SRAM_Fill %function
SRAM_Fill:
    // r0: fill byte, r1: nbytes, r2: offset
    CMP r1, #0
    BLE .LSRAM_Fill_Error
    CMP r2, #0
    BLE .LSRAM_Fill_Error
    MOV r12, r2  /* r12 = r2 = offset */
    AND r2, r2, r0
    CMP r2, r0
    BNE .LSRAM_Fill_Error  // if (fill_byte&0xFF)!=fill_byte return error
    MOV r2, r12  // r2 = offset again

    ADD r2, r2, r1  /* r2 = offset+size */
    
    MOV r3, #1
    LSL r3, r3, #16  /* r3 = 1<<16 = 0x10000 := SIZEOF(SRAM) */
    CMP r2, r3
    BGT .LSRAM_Fill_Error
    // Setup waitcnt
    MOV r3, #4  /* r3 = 0x0004 */
    MOV r2, #0x80
    LSL r2, r2, #2  /* r2 = 0x0080<<2 = 0x0200 */
    ORR r3, r3, r2  /* r3 = 0x0200 | 0x0004 = 0x0204 */
    LSL r2, r2, #17  /* r2 = 0x0200<<17 = 0x04000000 */
    ORR r3, r3, r2  /* r3 = 0x04000000 | 0x0204 = 0x04000204 = &REG_WAIT_CNT */
    LDRH r2, [r3]  /* r2 = *(volatile u16*)(&REG_WAIT_CNT) = REG_WAIT_CNT */
    PUSH { r2, r3 }  /* Save REG_WAIT_CNT and &REG_WAIT_CNT to stack for 
                        restoration after completing SRAM r/w ops. */
    PUSH { r4 }
    MOV r4, #3  /* r4 = 3 */
    ORR r2, r2, r4  /* r2 = REG_WAIT_CNT|3 
                       We need to set REG_WAIT_CNT bits 0-1 to 3, so we can just
                       ORR REG_WAIT_CNT's current value with the mask for this
                       register field, and ensure we've set the wait state field
                       to 3. */
    POP { r4 }
    STRH r2, [r3]  /* basically, REG_WAIT_CNT|=3 */

    MOV r2, r12  // restore r2 yet again

    MOV r3, #0xE0
    LSL r3, r3, #20  /*r3 = 0x0E000000 = SRAM */
    ORR r2, r2, r3  /* r2 = SRAM + offset = &SRAM[offset], 
                            and we know addr and ofs bits won't overlap so
                            we can just ORR */
.LSRAM_Fill_Loop:
        STRB r0, [r2]
        ADD r2, r2, #1
        SUB r1, r1, #1
        CMP r1, #0
        BNE .LSRAM_Fill_Loop
    POP { r2, r3 }
    STRH r2, [r3]
    MOV r0, #1
    BX lr 
.LSRAM_Fill_Error:
        MOV r0, #0
        BX lr
    .size SRAM_Fill, .-SRAM_Fill

    .section .iwram,"ax",%progbits
    .arm
    .align 2
    .global ChessGameloop_ISR_Handler
    .type ChessGameloop_ISR_Handler %function
    /* When ISR_Handler_Basic returns, it will have */
ChessGameloop_ISR_Handler:
    STMFD sp!, { lr }
    BL ISR_Handler_Basic
    LDMFD sp!, { lr }
    TST r1, #0x1000  /* Flagbit for Keypad IRQ firing in REG_IE */
    BXEQ lr
    LDR r2, =KEY_CURR
    LDRH r1, [r2]
    STRH r1, [r2, #2]
    MOV r0, #0x04000000
    ORR r0, r0, #0x130
    LDRH r1, [r0]
    MVN r1, r1
    STRH r1, [r2]
    BX lr
    .size ChessGameloop_ISR_Handler, .-ChessGameloop_ISR_Handler
    

    
    
    
    
